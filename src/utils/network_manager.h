#pragma once

#include "pico/stdlib.h"
#include <string>
#include <functional>

// Network connection states
enum class NetworkState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// Forward declarations to avoid header conflicts
struct tcp_pcb;
struct pbuf;
struct ip4_addr;
typedef struct ip4_addr ip_addr_t;

// Network manager class - isolated from other headers to avoid conflicts
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    // WiFi connection management
    bool init_wifi(const char* ssid, const char* password);
    bool is_connected() const;
    NetworkState get_state() const;
    void disconnect();
    
    // HTTP client functionality
    bool http_get(const std::string& url, std::function<void(const std::string&)> callback);
    
    // Must be called regularly in main loop
    void process();
    
    // Status information
    std::string get_status_message() const;
    int get_signal_strength() const;
    
private:
    NetworkState state;
    bool wifi_initialized;
    std::string wifi_ssid;
    std::string status_message;
    uint32_t last_connection_attempt;
    uint32_t connection_retry_delay;
    
    // HTTP state
    struct tcp_pcb* tcp_pcb;
    std::function<void(const std::string&)> response_callback;
    std::string response_buffer;
    
    // Internal methods
    void update_connection_state();
    bool attempt_wifi_connection();
    void cleanup_tcp_connection();
    void parse_url(const std::string& url, std::string& host, std::string& path);
    
    // Static callbacks (must be static for C-style callbacks)
    static void dns_callback(const char* name, const ip_addr_t* ipaddr, void* arg);
    static signed char tcp_connected_callback(void* arg, struct tcp_pcb* pcb, signed char err);
    static signed char tcp_recv_callback(void* arg, struct tcp_pcb* pcb, struct pbuf* p, signed char err);
};