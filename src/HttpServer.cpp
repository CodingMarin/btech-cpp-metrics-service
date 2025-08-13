#include "HttpServer.h"
#include "Config.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

HttpServer::HttpServer(std::shared_ptr<MetricsProcessor> processor)
    : metrics_processor(processor) {
    setup_routes();
}

void HttpServer::setup_routes() {
    setup_cors();
    setup_health_routes();
    setup_metrics_routes();
    setup_admin_routes();

    spdlog::info("HTTP routes configured");
}

void HttpServer::setup_cors() {
    server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Trace-ID");

        if (req.method == "OPTIONS") {
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
        });
}

void HttpServer::setup_health_routes() {
    server.Get("/health", [this](const httplib::Request&, httplib::Response& res) {
        auto health = metrics_processor->get_system_health();
        res.set_content(health.dump(), "application/json");
        });

    server.Get("/health/detailed", [this](const httplib::Request&, httplib::Response& res) {
        auto health = metrics_processor->get_system_health();
        auto metrics = metrics_processor->get_realtime_metrics();

        nlohmann::json detailed_health = {
            {"health", health},
            {"metrics_summary", {
                {"total_endpoints", metrics["endpoints"].size()},
                {"global_stats", metrics["global"]}
            }}
        };

        res.set_content(detailed_health.dump(), "application/json");
        });
}

void HttpServer::setup_metrics_routes() {
    server.Post("/metrics", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto json_data = nlohmann::json::parse(req.body);

            Metric metric;
            metric.endpoint = json_data["endpoint"];
            metric.method = json_data["method"];
            metric.duration_ms = json_data["duration"];
            metric.status = json_data["status"];
            metric.timestamp = json_data["timestamp"];
            metric.trace_id = json_data.value("trace_id", "");
            metric.service_name = json_data.value("service_name", "unknown");
            metric.user_id = json_data.value("user_id", "");
            metric.ip_address = json_data.value("ip_address", "");

            metrics_processor->process_metric(metric);

            nlohmann::json response = {
                {"success", true},
                {"message", "Metric processed successfully"}
            };

            res.set_content(response.dump(), "application/json");

        }
        catch (const std::exception& e) {
            spdlog::error("Error processing metric: {}", e.what());

            nlohmann::json error_response = {
                {"success", false},
                {"error", e.what()}
            };

            res.status = 400;
            res.set_content(error_response.dump(), "application/json");
        }
        });

    server.Get("/metrics/realtime", [this](const httplib::Request&, httplib::Response& res) {
        auto metrics = metrics_processor->get_realtime_metrics();
        res.set_content(metrics.dump(), "application/json");
        });

    server.Get(R"(/metrics/endpoint/(.+))", [this](const httplib::Request& req, httplib::Response& res) {
        std::string endpoint = req.matches[1];
        auto metrics = metrics_processor->get_endpoint_metrics(endpoint);
        res.set_content(metrics.dump(), "application/json");
        });
}

void HttpServer::setup_admin_routes() {
    server.Get("/info", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json info = {
            {"service", "btech-metrics-service"},
            {"version", "1.0.0"},
            {"description", "Real-time metrics processing for microservices"},
            {"endpoints", {
                "GET /health - Basic health check",
                "GET /health/detailed - Detailed health information",
                "POST /metrics - Submit metrics data",
                "GET /metrics/realtime - Get real-time metrics",
                "GET /metrics/endpoint/{endpoint} - Get specific endpoint metrics",
                "GET /info - Service information"
            }}
        };
        res.set_content(info.dump(2), "application/json");
        });
}

void HttpServer::start(int port) {
    spdlog::info("Starting HTTP server on port {}", port);
    spdlog::info("Available endpoints:");
    spdlog::info("  GET  /health - Health check");
    spdlog::info("  POST /metrics - Submit metrics");
    spdlog::info("  GET  /metrics/realtime - Real-time metrics");
    spdlog::info("  GET  /info - Service info");

    server.listen("0.0.0.0", port);
}

void HttpServer::stop() {
    server.stop();
    spdlog::info("HTTP server stopped");
}