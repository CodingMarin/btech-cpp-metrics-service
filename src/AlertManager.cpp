#include "AlertManager.h"
#include <spdlog/spdlog.h>

AlertManager::AlertManager(std::shared_ptr<MetricsProcessor> processor)
    : metrics_processor(processor) {
    setup_default_alerts();
}

void AlertManager::setup_default_alerts() {
    // Alerta por latencia alta
    metrics_processor->add_alert_handler([this](const Metric& metric) {
        if (metric.duration_ms > 5000) {
            log_alert(metric);
            // Aquí podrías agregar Slack/Email
            // send_slack_alert(metric);
        }
        });

    spdlog::info("Alert manager configured with default alerts");
}

void AlertManager::log_alert(const Metric& metric) {
    spdlog::critical("ALERT: High latency detected");
    spdlog::critical("   Endpoint: {} {}", metric.method, metric.endpoint);
    spdlog::critical("   Duration: {}ms", metric.duration_ms);
    spdlog::critical("   Status: {}", metric.status);
    spdlog::critical("   Service: {}", metric.service_name);
    spdlog::critical("   Trace ID: {}", metric.trace_id);
}

void AlertManager::send_slack_alert(const Metric& metric) {
    // TODO: Implementar integración con Slack
    spdlog::info("Would send Slack alert for: {} {}ms",
        metric.endpoint, metric.duration_ms);
}

void AlertManager::send_email_alert(const Metric& metric) {
    // TODO: Implementar integración con email
    spdlog::info("Would send email alert for: {} {}ms",
        metric.endpoint, metric.duration_ms);
}