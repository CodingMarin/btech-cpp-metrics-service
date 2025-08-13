#pragma once
#include "MetricsProcessor.h"
#include <string>
#include <vector>

class AlertManager {
private:
    std::shared_ptr<MetricsProcessor> metrics_processor;

public:
    explicit AlertManager(std::shared_ptr<MetricsProcessor> processor);
    void setup_default_alerts();

private:
    void send_slack_alert(const Metric& metric);
    void send_email_alert(const Metric& metric);
    void log_alert(const Metric& metric);
};