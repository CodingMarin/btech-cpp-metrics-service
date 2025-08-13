#pragma once
#include <httplib.h>
#include <memory>
#include "MetricsProcessor.h"

class HttpServer {
private:
    httplib::Server server;
    std::shared_ptr<MetricsProcessor> metrics_processor;

public:
    explicit HttpServer(std::shared_ptr<MetricsProcessor> processor);
    void setup_routes();
    void start(int port);
    void stop();

private:
    void setup_cors();
    void setup_health_routes();
    void setup_metrics_routes();
    void setup_admin_routes();
};