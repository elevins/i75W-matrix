#include "network_manager.h"

// Fix header conflicts - undef problematic macros before including lwIP
#ifdef local
#undef local
#endif

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h" 
#include "lwip/pbuf.h"
#include <cstring>
#include <algorithm>

// Connection retry settings
#define WIFI_CONNECT_TIMEOUT_MS 30000
#define RETRY_BASE_DELAY_MS 1000
#define RETRY_MAX_DELAY_MS 30000

NetworkManager::NetworkManager() 
    : state(NetworkState::DISCONNECTED)
    , wifi_initialized(false)
    , last_connection_attempt(0)
    , connection_retry_delay(RETRY_BASE_DELAY_MS)
    , tcp_pcb(nullptr)
{
    status_message = "Not initialized";
}

NetworkManager::~NetworkManager() {
    disconnect();
    if (wifi_initialized) {
        cyw43_arch_deinit();
    }
}

bool NetworkManager::init_wifi(const char* ssid, const char* password) {
    if (wifi_initialized) {
        disconnect();
        cyw43_arch_deinit();
        wifi_initialized = false;
    }
    
    // Initialize CYW43 WiFi chip
    if (cyw43_arch_init()) {
        status_message = "CYW43 init failed";
        state = NetworkState::ERROR;
        return false;
    }
    
    wifi_initialized = true;
    wifi_ssid = ssid;
    
    cyw43_arch_enable_sta_mode();
    
    status_message = "WiFi initialized";
    state = NetworkState::DISCONNECTED;
    
    printf("NetworkManager: WiFi initialized for SSID: %s\\n", ssid);
    
    // Start connection attempt
    state = NetworkState::CONNECTING;
    last_connection_attempt = to_ms_since_boot(get_absolute_time());
    
    // Attempt connection (non-blocking)
    int result = cyw43_arch_wifi_connect_async(ssid, password, CYW43_AUTH_WPA2_AES_PSK);
    
    if (result != 0) {
        status_message = "WiFi connect failed";
        state = NetworkState::ERROR;
        return false;
    }
    
    status_message = "Connecting to WiFi...";
    return true;
}

void NetworkManager::update_connection_state() {
    if (!wifi_initialized) {
        return;
    }
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    
    switch (state) {
        case NetworkState::CONNECTING:
            if (link_status == CYW43_LINK_UP) {
                state = NetworkState::CONNECTED;
                status_message = "WiFi connected";
                connection_retry_delay = RETRY_BASE_DELAY_MS; // Reset retry delay
                printf("NetworkManager: WiFi connected successfully\\n");
            } else if (link_status == CYW43_LINK_FAIL || link_status == CYW43_LINK_NONET) {
                state = NetworkState::ERROR;
                status_message = "WiFi connection failed";
                printf("NetworkManager: WiFi connection failed\\n");
            } else if (now - last_connection_attempt > WIFI_CONNECT_TIMEOUT_MS) {
                state = NetworkState::ERROR;
                status_message = "WiFi connection timeout";
                printf("NetworkManager: WiFi connection timeout\\n");
            }
            break;
            
        case NetworkState::CONNECTED:
            if (link_status != CYW43_LINK_UP) {
                state = NetworkState::DISCONNECTED;
                status_message = "WiFi disconnected";
                cleanup_tcp_connection();
                printf("NetworkManager: WiFi disconnected\\n");
            }
            break;
            
        case NetworkState::ERROR:
        case NetworkState::DISCONNECTED:
            // Auto-retry connection with exponential backoff
            if (now - last_connection_attempt > connection_retry_delay) {
                printf("NetworkManager: Attempting WiFi reconnection...\\n");
                state = NetworkState::CONNECTING;
                status_message = "Reconnecting to WiFi...";
                last_connection_attempt = now;
                
                cyw43_arch_wifi_connect_async(wifi_ssid.c_str(), "", CYW43_AUTH_WPA2_AES_PSK);
                
                // Exponential backoff
                connection_retry_delay = std::min(connection_retry_delay * 2, (uint32_t)RETRY_MAX_DELAY_MS);
            }
            break;
    }
}

bool NetworkManager::is_connected() const {
    return state == NetworkState::CONNECTED;
}

NetworkState NetworkManager::get_state() const {
    return state;
}

std::string NetworkManager::get_status_message() const {
    return status_message;
}

int NetworkManager::get_signal_strength() const {
    if (!wifi_initialized || !is_connected()) {
        return 0;
    }
    // CYW43 doesn't provide easy RSSI access, return basic indicator
    return is_connected() ? 75 : 0;
}

void NetworkManager::process() {
    if (wifi_initialized) {
        cyw43_arch_poll();
        update_connection_state();
    }
}

void NetworkManager::disconnect() {
    cleanup_tcp_connection();
    
    if (wifi_initialized && state != NetworkState::DISCONNECTED) {
        cyw43_arch_disable_sta_mode();
        state = NetworkState::DISCONNECTED;
        status_message = "Disconnected";
    }
}

void NetworkManager::cleanup_tcp_connection() {
    if (tcp_pcb) {
        tcp_close(tcp_pcb);
        tcp_pcb = nullptr;
    }
    response_callback = nullptr;
    response_buffer.clear();
}

void NetworkManager::parse_url(const std::string& url, std::string& host, std::string& path) {
    // Simple URL parsing - assumes http:// or https://
    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos) {
        host = "";
        path = "";
        return;
    }
    
    size_t host_start = protocol_end + 3;
    size_t path_start = url.find('/', host_start);
    
    if (path_start == std::string::npos) {
        host = url.substr(host_start);
        path = "/";
    } else {
        host = url.substr(host_start, path_start - host_start);
        path = url.substr(path_start);
    }
}

struct http_request_data {
    NetworkManager* manager;
    std::string host;
    std::string path;
};

void NetworkManager::dns_callback(const char* name, const ip_addr_t* ipaddr, void* arg) {
    http_request_data* req_data = (http_request_data*)arg;
    
    if (ipaddr == nullptr) {
        printf("NetworkManager: DNS lookup failed for %s\\n", name);
        delete req_data;
        return;
    }
    
    // Create TCP connection
    struct tcp_pcb* pcb = tcp_new();
    if (pcb == nullptr) {
        printf("NetworkManager: Failed to create TCP PCB\\n");
        delete req_data;
        return;
    }
    
    req_data->manager->tcp_pcb = pcb;
    tcp_arg(pcb, req_data);
    
    err_t err = tcp_connect(pcb, ipaddr, 80, tcp_connected_callback);
    if (err != ERR_OK) {
        printf("NetworkManager: TCP connect failed: %d\\n", err);
        tcp_close(pcb);
        req_data->manager->tcp_pcb = nullptr;
        delete req_data;
    }
}

signed char NetworkManager::tcp_connected_callback(void* arg, struct tcp_pcb* pcb, signed char err) {
    http_request_data* req_data = (http_request_data*)arg;
    
    if (err != ERR_OK) {
        printf("NetworkManager: TCP connection failed: %d\\n", err);
        delete req_data;
        return err;
    }
    
    // Build HTTP GET request
    std::string request = "GET " + req_data->path + " HTTP/1.1\\r\\n";
    request += "Host: " + req_data->host + "\\r\\n";
    request += "Connection: close\\r\\n";
    request += "User-Agent: PicoW-Weather/1.0\\r\\n";
    request += "\\r\\n";
    
    // Send HTTP request
    err_t write_err = tcp_write(pcb, request.c_str(), request.length(), TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        printf("NetworkManager: TCP write failed: %d\\n", write_err);
        delete req_data;
        return write_err;
    }
    
    tcp_output(pcb);
    
    // Set up receive callback
    tcp_recv(pcb, tcp_recv_callback);
    tcp_arg(pcb, req_data->manager);
    
    delete req_data;
    return ERR_OK;
}

signed char NetworkManager::tcp_recv_callback(void* arg, struct tcp_pcb* pcb, struct pbuf* p, signed char err) {
    NetworkManager* manager = (NetworkManager*)arg;
    
    if (p == nullptr) {
        // Connection closed
        tcp_close(pcb);
        manager->tcp_pcb = nullptr;
        
        // Process response - skip HTTP headers
        std::string& response = manager->response_buffer;
        size_t body_start = response.find("\\r\\n\\r\\n");
        if (body_start != std::string::npos) {
            std::string body = response.substr(body_start + 4);
            if (manager->response_callback) {
                manager->response_callback(body);
            }
        }
        
        manager->response_buffer.clear();
        manager->response_callback = nullptr;
        return ERR_OK;
    }
    
    // Append received data to buffer
    char* data = (char*)p->payload;
    manager->response_buffer.append(data, p->len);
    
    tcp_recved(pcb, p->len);
    pbuf_free(p);
    
    return ERR_OK;
}

bool NetworkManager::http_get(const std::string& url, std::function<void(const std::string&)> callback) {
    if (!is_connected()) {
        printf("NetworkManager: HTTP GET failed - not connected\\n");
        return false;
    }
    
    if (tcp_pcb != nullptr) {
        printf("NetworkManager: HTTP GET failed - connection busy\\n");
        return false;
    }
    
    response_callback = callback;
    response_buffer.clear();
    
    std::string host, path;
    parse_url(url, host, path);
    
    if (host.empty()) {
        printf("NetworkManager: Invalid URL: %s\\n", url.c_str());
        return false;
    }
    
    printf("NetworkManager: HTTP GET %s%s\\n", host.c_str(), path.c_str());
    
    // Prepare request data
    http_request_data* req_data = new http_request_data();
    req_data->manager = this;
    req_data->host = host;
    req_data->path = path;
    
    // Resolve DNS
    ip_addr_t server_ip;
    err_t err = dns_gethostbyname(host.c_str(), &server_ip, dns_callback, req_data);
    
    if (err == ERR_OK) {
        // IP was cached, call callback directly
        dns_callback(host.c_str(), &server_ip, req_data);
    } else if (err != ERR_INPROGRESS) {
        printf("NetworkManager: DNS lookup failed immediately: %d\\n", err);
        delete req_data;
        return false;
    }
    
    return true;
}