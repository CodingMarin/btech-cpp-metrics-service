#include <iostream>
#include <memory>
#include <thread>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Config.h"
#include "MetricsProcessor.h"
#include "HttpServer.h"
#include "AlertManager.h"

Config g_config;
std::unique_ptr<HttpServer> g_server;

void setup_logging() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("metrics_service", console_sink);

    if (g_config.log_level == "debug") {
        logger->set_level(spdlog::level::debug);
    }
    else if (g_config.log_level == "warn") {
        logger->set_level(spdlog::level::warn);
    }
    else {
        logger->set_level(spdlog::level::info);
    }

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [%t] %v");

    spdlog::set_default_logger(logger);
    spdlog::info("Logging system initialized (level: {})", g_config.log_level);
}

void signal_handler(int signal) {
    spdlog::info("Received signal {}, shutting down gracefully...", signal);
    if (g_server) {
        g_server->stop();
    }
    exit(0);
}

void setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

int main() {
    try {
        setup_logging();
        setup_signal_handlers();

        spdlog::info("Starting BTECH Metrics Service...");
        spdlog::info("Configuration:");
        spdlog::info("   Port: {}", g_config.port);
        spdlog::info("   Redis: {}:{}", g_config.redis_host, g_config.redis_port);
        spdlog::info("   Log Level: {}", g_config.log_level);
        spdlog::info("   Alert Threshold: {}ms", g_config.alert_threshold_ms);

        auto metrics_processor = std::make_shared<MetricsProcessor>();

        auto alert_manager = std::make_unique<AlertManager>(metrics_processor);

        g_server = std::make_unique<HttpServer>(metrics_processor);

        spdlog::info("BTECH Metrics Service ready!");
        spdlog::info("Service accessible at: http://localhost:{}", g_config.port);
        spdlog::info("Dashboard: http://localhost:3000");
        spdlog::info("Health check: http://localhost:{}/health", g_config.port);

        // Iniciar servidor (esto bloquea el hilo principal)
        g_server->start(g_config.port);

    }
    catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}