#pragma once

#include <string>
#include <functional>
#include <queue>

// Forward declaration of opaque handle from C wrapper
typedef void* https_handle_t;
typedef void* https_request_t;

struct HTTPSRequest {
    std::string hostname;
    std::string path;
    std::function<void(const std::string& response)> callback;
    std::function<void(const std::string& error)> error_callback;
    int timeout_ms;
};

class HTTPSService {
public:
    HTTPSService();
    ~HTTPSService();
    
    // Initialize WiFi and HTTPS service
    bool initialize();
    bool initialize_with_config(const std::string& ssid, const std::string& password, int timeout_ms = 30000);
    
    // Queue an HTTPS request
    void request(const std::string& hostname, const std::string& path,
                std::function<void(const std::string& response)> callback,
                std::function<void(const std::string& error)> error_callback,
                int timeout_ms = 30000);
    
    // Process queued requests - call this in main loop
    void poll();
    
    // Check if WiFi is connected
    bool is_connected() const;
    
    // Get connection status string
    std::string get_status() const;
    
    // Get number of pending requests
    int get_pending_count() const;

private:
    // Opaque handle to C implementation - no lwIP types exposed
    https_handle_t handle_;
    
    // Queue for pending C++ requests that need to be converted to C requests
    std::queue<HTTPSRequest> request_queue_;
    
    // Track active requests for callback management
    struct ActiveRequest {
        https_request_t c_request;
        std::function<void(const std::string& response)> callback;
        std::function<void(const std::string& error)> error_callback;
        bool in_use;
    };
    static const int MAX_ACTIVE_REQUESTS = 8;
    ActiveRequest active_requests_[MAX_ACTIVE_REQUESTS];
    
    // Find free active request slot
    ActiveRequest* find_free_slot();
    ActiveRequest* find_active_request(https_request_t c_request);
    void cleanup_active_request(ActiveRequest* active_req);
    
    // Static C callback wrappers - bridge C callbacks to C++ callbacks
    static void static_response_callback(const char* response, int response_len, void* user_data);
    static void static_error_callback(const char* error_msg, void* user_data);
    
    // Process queued requests
    void process_request_queue();
};