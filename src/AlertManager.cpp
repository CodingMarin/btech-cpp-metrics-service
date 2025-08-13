#include "AlertManager.h"
#include "Config.h"
#include <spdlog/spdlog.h>

AlertManager::AlertManager(std::shared_ptr<MetricsProcessor> processor)
    : metrics_processor(processor) {
    setup_default_alerts();
}

void AlertManager::setup_default_alerts() {
    metrics_processor->add_alert_handler([this](const Metric& metric) {
        log_alert(metric);

        // Aquí puedes agregar más tipos de alertas
        // send_slack_alert(metric);
        // send_email_alert(metric);
    });

    spdlog::info("Alert handlers configured");
}

void AlertManager::log_alert(const Metric& metric) {
    spdlog::warn("ALERT: High latency detected - Endpoint: {} {}ms - Status: {}", 
                 metric.endpoint, metric.duration_ms, metric.status);
}

void AlertManager::send_slack_alert(const Metric& metric) {
    // Implementación de alerta Slack
    spdlog::info("Would send Slack alert for endpoint: {}", metric.endpoint);
}

void AlertManager::send_email_alert(const Metric& metric) {
    // Implementación de alerta por email
    spdlog::info("Would send email alert for endpoint: {}", metric.endpoint);
}