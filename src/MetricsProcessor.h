#pragma once
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>

struct Metric {
    std::string endpoint;
    std::string method;
    uint64_t duration_ms;
    std::string status;
    uint64_t timestamp;
    std::string trace_id;
    std::string service_name;
    std::string user_id;
    std::string ip_address;
};

class CircularBuffer {
private:
    std::vector<double> buffer;
    size_t head = 0;
    size_t count = 0;
    size_t capacity;
    mutable std::mutex mutex;

public:
    explicit CircularBuffer(size_t size) : capacity(size) {
        buffer.resize(capacity);
    }

    void push(double value) {
        std::lock_guard<std::mutex> lock(mutex);
        buffer[head] = value;
        head = (head + 1) % capacity;
        if (count < capacity) count++;
    }

    std::vector<double> get_sorted_values() const {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<double> values;
        values.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            size_t idx = (head - count + i + capacity) % capacity;
            values.push_back(buffer[idx]);
        }

        std::sort(values.begin(), values.end());
        return values;
    }

    double percentile(double p) const {
        auto values = get_sorted_values();
        if (values.empty()) return 0.0;

        size_t index = static_cast<size_t>(p * (values.size() - 1));
        return values[index];
    }

    double average() const {
        auto values = get_sorted_values();
        if (values.empty()) return 0.0;

        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / values.size();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return count;
    }
};

struct EndpointStats {
    std::unique_ptr<CircularBuffer> latency_buffer;
    std::atomic<uint64_t> request_count{ 0 };
    std::atomic<uint64_t> error_count{ 0 };
    std::atomic<uint64_t> success_count{ 0 };
    std::chrono::steady_clock::time_point last_request;

    EndpointStats() : latency_buffer(std::make_unique<CircularBuffer>(1000)) {
        last_request = std::chrono::steady_clock::now();
    }
};

class MetricsProcessor {
private:
    std::unordered_map<std::string, std::unique_ptr<EndpointStats>> endpoint_stats;
    std::mutex stats_mutex;

    std::atomic<uint64_t> total_requests{ 0 };
    std::atomic<uint64_t> total_errors{ 0 };
    std::chrono::steady_clock::time_point start_time;

    std::vector<std::function<void(const Metric&)>> alert_handlers;

public:
    MetricsProcessor();
    void process_metric(const Metric& metric);
    nlohmann::json get_realtime_metrics();
    nlohmann::json get_endpoint_metrics(const std::string& endpoint);
    nlohmann::json get_system_health();
    bool is_anomaly(const Metric& metric);
    void add_alert_handler(std::function<void(const Metric&)> handler);

private:
    std::string make_endpoint_key(const std::string& method, const std::string& endpoint);
    void trigger_alerts(const Metric& metric);
};