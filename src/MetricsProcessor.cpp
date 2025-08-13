#include "MetricsProcessor.h"
#include "Config.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>

MetricsProcessor::MetricsProcessor() {
    start_time = std::chrono::steady_clock::now();
    spdlog::info("MetricsProcessor initialized");
}

void MetricsProcessor::process_metric(const Metric& metric) {
    std::string key = make_endpoint_key(metric.method, metric.endpoint);

    total_requests++;

    std::unique_ptr<EndpointStats>* stats_ptr;
    {
        std::lock_guard<std::mutex> lock(stats_mutex);
        if (endpoint_stats.find(key) == endpoint_stats.end()) {
            endpoint_stats[key] = std::make_unique<EndpointStats>();
        }
        stats_ptr = &endpoint_stats[key];
    }

    auto& stats = **stats_ptr;

    stats.request_count++;
    stats.last_request = std::chrono::steady_clock::now();

    if (metric.status == "success" || metric.status == "200") {
        stats.success_count++;
    }
    else {
        stats.error_count++;
        total_errors++;
    }

    stats.latency_buffer->push(static_cast<double>(metric.duration_ms));

    if (is_anomaly(metric)) {
        trigger_alerts(metric);
    }

    spdlog::debug("Processed metric: {} - {}ms - {}", key, metric.duration_ms, metric.status);
}

nlohmann::json MetricsProcessor::get_realtime_metrics() {
    nlohmann::json result;
    result["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time).count();

    result["global"] = {
        {"total_requests", total_requests.load()},
        {"total_errors", total_errors.load()},
        {"error_rate", total_requests > 0 ?
            static_cast<double>(total_errors) / total_requests * 100 : 0.0},
        {"uptime_seconds", uptime},
        {"requests_per_second", uptime > 0 ?
            static_cast<double>(total_requests) / uptime : 0.0}
    };

    result["endpoints"] = nlohmann::json::array();

    std::lock_guard<std::mutex> lock(stats_mutex);
    for (const auto& [endpoint, stats] : endpoint_stats) {
        nlohmann::json endpoint_data;
        endpoint_data["endpoint"] = endpoint;
        endpoint_data["request_count"] = stats->request_count.load();
        endpoint_data["error_count"] = stats->error_count.load();
        endpoint_data["success_count"] = stats->success_count.load();

        auto error_rate = stats->request_count > 0 ?
            static_cast<double>(stats->error_count) / stats->request_count * 100 : 0.0;
        endpoint_data["error_rate"] = error_rate;

        auto buffer_size = stats->latency_buffer->size();
        if (buffer_size > 0) {
            endpoint_data["latency"] = {
                {"count", buffer_size},
                {"avg", stats->latency_buffer->average()},
                {"p50", stats->latency_buffer->percentile(0.5)},
                {"p95", stats->latency_buffer->percentile(0.95)},
                {"p99", stats->latency_buffer->percentile(0.99)}
            };
        }
        else {
            endpoint_data["latency"] = nullptr;
        }

        auto last_request_ago = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - stats->last_request).count();
        endpoint_data["last_request_seconds_ago"] = last_request_ago;

        result["endpoints"].push_back(endpoint_data);
    }

    return result;
}

nlohmann::json MetricsProcessor::get_system_health() {
    nlohmann::json health;

    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start_time).count();

    auto error_rate = total_requests > 0 ?
        static_cast<double>(total_errors) / total_requests * 100 : 0.0;

    std::string status;
    if (error_rate > 10.0) {
        status = "critical";
    }
    else if (error_rate > 5.0) {
        status = "warning";
    }
    else {
        status = "healthy";
    }

    health = {
        {"status", status},
        {"uptime_seconds", uptime},
        {"total_requests", total_requests.load()},
        {"total_errors", total_errors.load()},
        {"error_rate_percent", error_rate},
        {"endpoints_count", endpoint_stats.size()},
        {"service", "metrics-service"},
        {"version", "1.0.0"}
    };

    return health;
}

bool MetricsProcessor::is_anomaly(const Metric& metric) {
    if (metric.duration_ms > g_config.alert_threshold_ms) {
        return true;
    }

    std::string key = make_endpoint_key(metric.method, metric.endpoint);
    std::lock_guard<std::mutex> lock(stats_mutex);

    auto it = endpoint_stats.find(key);
    if (it != endpoint_stats.end()) {
        auto& stats = it->second;
        auto error_rate = stats->request_count > 10 ?
            static_cast<double>(stats->error_count) / stats->request_count * 100 : 0.0;

        if (error_rate > 20.0) {
            return true;
        }
    }

    return false;
}

void MetricsProcessor::add_alert_handler(std::function<void(const Metric&)> handler) {
    alert_handlers.push_back(std::move(handler));
}

std::string MetricsProcessor::make_endpoint_key(const std::string& method, const std::string& endpoint) {
    return method + ":" + endpoint;
}

void MetricsProcessor::trigger_alerts(const Metric& metric) {
    spdlog::warn("Anomaly detected: {} {}ms - {}",
        make_endpoint_key(metric.method, metric.endpoint),
        metric.duration_ms,
        metric.status);

    for (const auto& handler : alert_handlers) {
        try {
            handler(metric);
        }
        catch (const std::exception& e) {
            spdlog::error("Error in alert handler: {}", e.what());
        }
    }
}