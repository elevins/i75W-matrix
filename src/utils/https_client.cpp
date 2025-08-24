#include "https_client.h"
#include "lwip/pbuf.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "mbedtls/ssl.h"
#include <cstring>
#include <iostream>

HttpsClient::HttpsClient() : wifi_connected(false), tls_pcb(nullptr), tls_config(nullptr) {
    // Create TLS configuration
    tls_config = altcp_tls_create_config_client(NULL, 0);
}

HttpsClient::~HttpsClient() {
    if (tls_pcb) {
        altcp_close(tls_pcb);
        tls_pcb = nullptr;
    }
    if (tls_config) {
        altcp_tls_free_config(tls_config);
        tls_config = nullptr;
    }
}

bool HttpsClient::init(const char* ssid, const char* password) {
    // Initialize WiFi
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43\n");
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    
    printf("Connecting to WiFi: %s\n", ssid);
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to WiFi\n");
        return false;
    }
    
    printf("Connected to WiFi\n");
    wifi_connected = true;
    return true;
}

bool HttpsClient::is_connected() {
    return wifi_connected && (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP);
}

void HttpsClient::process() {
    cyw43_arch_poll();
}

void HttpsClient::parse_url(const std::string& url, std::string& host, std::string& path) {
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

struct https_request_data {
    HttpsClient* client;
    std::string host;
    std::string path;
};

err_t HttpsClient::tls_connected_callback(void* arg, struct altcp_pcb* pcb, err_t err) {
    https_request_data* req_data = (https_request_data*)arg;
    
    if (err != ERR_OK) {
        printf("TLS connection failed: %d\n", err);
        delete req_data;
        return err;
    }
    
    // Build HTTPS GET request
    std::string request = "GET " + req_data->path + " HTTP/1.1\r\n";
    request += "Host: " + req_data->host + "\r\n";
    request += "Connection: close\r\n";
    request += "User-Agent: PicoW-Weather/1.0\r\n";
    request += "\r\n";
    
    // Send HTTPS request
    err_t write_err = altcp_write(pcb, request.c_str(), request.length(), TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        printf("TLS write failed: %d\n", write_err);
        delete req_data;
        return write_err;
    }
    
    altcp_output(pcb);
    
    // Set up receive callback
    altcp_recv(pcb, tls_recv_callback);
    altcp_arg(pcb, req_data->client);
    
    delete req_data;
    return ERR_OK;
}

err_t HttpsClient::tls_recv_callback(void* arg, struct altcp_pcb* pcb, struct pbuf* p, err_t err) {
    HttpsClient* client = (HttpsClient*)arg;
    
    if (p == nullptr) {
        // Connection closed
        altcp_close(pcb);
        client->tls_pcb = nullptr;
        
        // Process response - skip HTTP headers
        std::string& response = client->response_buffer;
        size_t body_start = response.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            std::string body = response.substr(body_start + 4);
            if (client->response_callback) {
                client->response_callback(body);
            }
        }
        
        client->response_buffer.clear();
        return ERR_OK;
    }
    
    // Append received data to buffer
    char* data = (char*)p->payload;
    client->response_buffer.append(data, p->len);
    
    altcp_recved(pcb, p->len);
    pbuf_free(p);
    
    return ERR_OK;
}

void HttpsClient::dns_callback(const char* name, const ip_addr_t* ipaddr, void* arg) {
    https_request_data* req_data = (https_request_data*)arg;
    
    if (ipaddr == nullptr) {
        printf("DNS lookup failed for %s\n", name);
        delete req_data;
        return;
    }
    
    // Create TLS connection
    struct altcp_pcb* pcb = altcp_tls_new(req_data->client->tls_config, IPADDR_TYPE_ANY);
    if (pcb == nullptr) {
        printf("Failed to create TLS PCB\n");
        delete req_data;
        return;
    }
    
    // Set hostname for SNI
    mbedtls_ssl_set_hostname((mbedtls_ssl_context*)altcp_tls_context(pcb), req_data->host.c_str());
    
    req_data->client->tls_pcb = pcb;
    altcp_arg(pcb, req_data);
    
    err_t err = altcp_connect(pcb, ipaddr, 443, tls_connected_callback);
    if (err != ERR_OK) {
        printf("TLS connect failed: %d\n", err);
        altcp_close(pcb);
        req_data->client->tls_pcb = nullptr;
        delete req_data;
    }
}

bool HttpsClient::get(const std::string& url, std::function<void(const std::string&)> callback) {
    if (!is_connected()) {
        return false;
    }
    
    response_callback = callback;
    response_buffer.clear();
    
    std::string host, path;
    parse_url(url, host, path);
    
    if (host.empty()) {
        printf("Invalid URL: %s\n", url.c_str());
        return false;
    }
    
    printf("Making HTTPS request to %s%s\n", host.c_str(), path.c_str());
    
    // Prepare request data
    https_request_data* req_data = new https_request_data();
    req_data->client = this;
    req_data->host = host;
    req_data->path = path;
    
    // Resolve DNS
    ip_addr_t server_ip;
    err_t err = dns_gethostbyname(host.c_str(), &server_ip, dns_callback, req_data);
    
    if (err == ERR_OK) {
        // IP was cached, call callback directly
        dns_callback(host.c_str(), &server_ip, req_data);
    } else if (err != ERR_INPROGRESS) {
        printf("DNS lookup failed immediately: %d\n", err);
        delete req_data;
        return false;
    }
    
    return true;
}