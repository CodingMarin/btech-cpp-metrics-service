#pragma once
#include <string>
#include <cstdlib>

struct Config {
    int port;
    std::string redis_host;
    int redis_port;
    int max_metrics_buffer;
    bool enable_alerts;
    std::string log_level;
    int alert_threshold_ms;

    Config() {
        port = std::getenv("PORT") ? std::atoi(std::getenv("PORT")) : 8080;
        redis_host = std::getenv("REDIS_HOST") ? std::getenv("REDIS_HOST") : "localhost";
        redis_port = std::getenv("REDIS_PORT") ? std::atoi(std::getenv("REDIS_PORT")) : 6379;
        max_metrics_buffer = 10000;
        enable_alerts = true;
        log_level = std::getenv("LOG_LEVEL") ? std::getenv("LOG_LEVEL") : "info";
        alert_threshold_ms = 5000;
    }
};

extern Config g_config;