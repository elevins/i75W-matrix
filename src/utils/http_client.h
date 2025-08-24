#pragma once

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include <string>
#include <functional>

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // Initialize WiFi and HTTP client
    bool init(const char* ssid, const char* password);
    
    // Make HTTP GET request (non-blocking) - supports both HTTP and HTTPS
    bool get(const std::string& url, std::function<void(const std::string&)> callback);
    
    // Check if WiFi is connected
    bool is_connected();
    
    // Process pending network operations (call in main loop)
    void process();
    
private:
    bool wifi_connected;
    struct altcp_pcb* altcp_pcb;
    std::function<void(const std::string&)> response_callback;
    std::string response_buffer;
    
    // Internal callback functions
    static err_t altcp_connected_callback(void* arg, struct altcp_pcb* pcb, err_t err);
    static err_t altcp_recv_callback(void* arg, struct altcp_pcb* pcb, struct pbuf* p, err_t err);
    static void dns_callback(const char* name, const ip_addr_t* ipaddr, void* arg);
    
    void parse_url(const std::string& url, std::string& host, std::string& path, bool& is_https);
};