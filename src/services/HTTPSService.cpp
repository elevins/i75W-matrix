#include "HTTPSService.hpp"
#include "https_c_wrapper.h"
#include <cstdio>
#include <cstring>

HTTPSService::HTTPSService() : handle_(nullptr) {
    // Initialize active requests array
    for (int i = 0; i < MAX_ACTIVE_REQUESTS; i++) {
        active_requests_[i].in_use = false;
        active_requests_[i].c_request = nullptr;
    }
}

HTTPSService::~HTTPSService() {
    if (handle_) {
        https_cleanup(handle_);
        handle_ = nullptr;
    }
}

bool HTTPSService::initialize() {
    return initialize_with_config("EddyBsHouse", "swiftwater496", 30000);
}

bool HTTPSService::initialize_with_config(const std::string& ssid, const std::string& password, int timeout_ms) {
    // Initialize C wrapper
    handle_ = https_init();
    if (!handle_) {
        printf("Failed to initialize HTTPS service\n");
        return false;
    }
    
    // Configure WiFi
    https_wifi_config_t wifi_config;
    wifi_config.ssid = ssid.c_str();
    wifi_config.password = password.c_str();
    wifi_config.timeout_ms = timeout_ms;
    
    return https_wifi_init(handle_, &wifi_config);
}

void HTTPSService::request(const std::string& hostname, const std::string& path,
                          std::function<void(const std::string& response)> callback,
                          std::function<void(const std::string& error)> error_callback,
                          int timeout_ms) {
    HTTPSRequest req;
    req.hostname = hostname;
    req.path = path;
    req.callback = callback;
    req.error_callback = error_callback;
    req.timeout_ms = timeout_ms;
    
    request_queue_.push(req);
}

void HTTPSService::poll() {
    if (!handle_) return;
    
    // Poll C wrapper
    https_poll(handle_, 0);
    
    // Process any queued requests
    process_request_queue();
}

bool HTTPSService::is_connected() const {
    if (!handle_) return false;
    return https_is_connected(handle_);
}

std::string HTTPSService::get_status() const {
    if (!handle_) return "Not initialized";
    
    char buffer[128];
    https_get_status(handle_, buffer, sizeof(buffer));
    return std::string(buffer);
}

int HTTPSService::get_pending_count() const {
    if (!handle_) return 0;
    return https_get_pending_count(handle_);
}

HTTPSService::ActiveRequest* HTTPSService::find_free_slot() {
    for (int i = 0; i < MAX_ACTIVE_REQUESTS; i++) {
        if (!active_requests_[i].in_use) {
            return &active_requests_[i];
        }
    }
    return nullptr;
}

HTTPSService::ActiveRequest* HTTPSService::find_active_request(https_request_t c_request) {
    for (int i = 0; i < MAX_ACTIVE_REQUESTS; i++) {
        if (active_requests_[i].in_use && active_requests_[i].c_request == c_request) {
            return &active_requests_[i];
        }
    }
    return nullptr;
}

void HTTPSService::cleanup_active_request(ActiveRequest* active_req) {
    if (active_req) {
        active_req->in_use = false;
        active_req->c_request = nullptr;
        active_req->callback = nullptr;
        active_req->error_callback = nullptr;
    }
}

void HTTPSService::static_response_callback(const char* response, int response_len, void* user_data) {
    ActiveRequest* active_req = static_cast<ActiveRequest*>(user_data);
    if (active_req && active_req->in_use && active_req->callback) {
        std::string response_str(response, response_len);
        active_req->callback(response_str);
    }
    
    // Cleanup the active request
    if (active_req) {
        active_req->in_use = false;
        active_req->c_request = nullptr;
    }
}

void HTTPSService::static_error_callback(const char* error_msg, void* user_data) {
    ActiveRequest* active_req = static_cast<ActiveRequest*>(user_data);
    if (active_req && active_req->in_use && active_req->error_callback) {
        std::string error_str(error_msg);
        active_req->error_callback(error_str);
    }
    
    // Cleanup the active request
    if (active_req) {
        active_req->in_use = false;
        active_req->c_request = nullptr;
    }
}

void HTTPSService::process_request_queue() {
    if (!handle_) return;
    
    while (!request_queue_.empty()) {
        // Find a free slot for tracking this request
        ActiveRequest* active_req = find_free_slot();
        if (!active_req) {
            // No free slots, wait for some requests to complete
            break;
        }
        
        HTTPSRequest req = request_queue_.front();
        request_queue_.pop();
        
        // Set up active request tracking
        active_req->callback = req.callback;
        active_req->error_callback = req.error_callback;
        active_req->in_use = true;
        
        // Configure C request
        https_request_config_t c_config;
        c_config.hostname = req.hostname.c_str();
        c_config.path = req.path.c_str();
        c_config.response_callback = static_response_callback;
        c_config.error_callback = static_error_callback;
        c_config.user_data = active_req;
        c_config.timeout_ms = req.timeout_ms;
        
        // Submit request to C wrapper
        https_request_t c_request = https_request(handle_, &c_config);
        if (c_request) {
            active_req->c_request = c_request;
            printf("HTTPS request queued successfully\n");
        } else {
            // Request failed immediately
            if (active_req->error_callback) {
                active_req->error_callback("Failed to queue request");
            }
            cleanup_active_request(active_req);
        }
    }
}