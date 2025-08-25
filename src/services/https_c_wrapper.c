#include "https_c_wrapper.h"

// Include all lwIP/mbedTLS headers ONLY in this C file
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "mbedtls/ssl.h"
#include "mbedtls/version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PENDING_REQUESTS 8
#define MAX_RESPONSE_SIZE 4096
#define DEFAULT_TIMEOUT_MS 30000

// Internal request structure
typedef struct https_request_internal {
    https_request_config_t config;
    struct altcp_pcb* pcb;
    struct altcp_tls_config* tls_config;
    char* hostname_copy;
    char* path_copy;
    char* response_buffer;
    int response_len;
    int response_capacity;
    bool in_use;
    bool connection_established;
    bool request_sent;
    uint32_t start_time;
} https_request_internal_t;

// Internal HTTPS service structure
typedef struct https_service_internal {
    https_state_t state;
    bool wifi_initialized;
    bool wifi_connected;
    https_request_internal_t requests[MAX_PENDING_REQUESTS];
    int active_request_count;
    https_wifi_config_t wifi_config;
    char* wifi_ssid_copy;
    char* wifi_password_copy;
} https_service_internal_t;

// Helper function to get current time in milliseconds
static uint32_t get_time_ms(void) {
    return time_us_32() / 1000;
}

// Helper function to find free request slot
static https_request_internal_t* find_free_request_slot(https_service_internal_t* service) {
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (!service->requests[i].in_use) {
            return &service->requests[i];
        }
    }
    return NULL;
}

// Helper function to find request by PCB
static https_request_internal_t* find_request_by_pcb(https_service_internal_t* service, struct altcp_pcb* pcb) {
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (service->requests[i].in_use && service->requests[i].pcb == pcb) {
            return &service->requests[i];
        }
    }
    return NULL;
}

// Helper function to cleanup request
static void cleanup_request(https_request_internal_t* request) {
    if (request->pcb) {
        altcp_close(request->pcb);
        request->pcb = NULL;
    }
    if (request->tls_config) {
        altcp_tls_free_config(request->tls_config);
        request->tls_config = NULL;
    }
    if (request->hostname_copy) {
        free(request->hostname_copy);
        request->hostname_copy = NULL;
    }
    if (request->path_copy) {
        free(request->path_copy);
        request->path_copy = NULL;
    }
    if (request->response_buffer) {
        free(request->response_buffer);
        request->response_buffer = NULL;
    }
    request->in_use = false;
    request->connection_established = false;
    request->request_sent = false;
    request->response_len = 0;
}

// ALTCP callback functions
static const char* get_altcp_error_string(err_t err) {
    switch(err) {
        case ERR_MEM: return "Out of memory";
        case ERR_BUF: return "Buffer error";
        case ERR_TIMEOUT: return "Timeout";
        case ERR_RTE: return "Routing problem";
        case ERR_INPROGRESS: return "Operation in progress";
        case ERR_VAL: return "Illegal value";
        case ERR_WOULDBLOCK: return "Would block";
        case ERR_USE: return "Address in use";
        case ERR_ALREADY: return "Already connected";
        case ERR_ISCONN: return "Already connected";
        case ERR_CONN: return "Not connected";
        case ERR_IF: return "Low-level error";
        case ERR_ABRT: return "Connection aborted";
        case ERR_RST: return "Connection reset";
        case ERR_CLSD: return "TLS handshake failed (connection closed during handshake)";
        case ERR_ARG: return "Illegal argument";
        default: return "Unknown error";
    }
}

static void altcp_error_callback(void* arg, err_t err) {
    https_request_internal_t* request = (https_request_internal_t*)arg;
    if (request && request->in_use) {
        const char* error_str = get_altcp_error_string(err);
        printf("ALTCP error %d: %s\n", err, error_str);
        if (request->config.error_callback) {
            request->config.error_callback(error_str, request->config.user_data);
        }
        cleanup_request(request);
    }
}

static err_t altcp_connected_callback(void* arg, struct altcp_pcb* pcb, err_t err) {
    https_request_internal_t* request = (https_request_internal_t*)arg;
    
    if (err != ERR_OK || !request || !request->in_use) {
        const char* error_str = get_altcp_error_string(err);
        printf("TLS connection failed: %d (%s)\n", err, error_str);
        if (request && request->config.error_callback) {
            request->config.error_callback(error_str, request->config.user_data);
        }
        if (request) cleanup_request(request);
        return ERR_OK;
    }
    
    printf("TLS handshake completed successfully! Sending HTTP request to %s\n", request->hostname_copy);
    request->connection_established = true;
    
    // Build HTTP request
    char* http_request;
    int request_len = snprintf(NULL, 0, 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        request->path_copy, request->hostname_copy) + 1;
    
    http_request = malloc(request_len);
    if (!http_request) {
        if (request->config.error_callback) {
            request->config.error_callback("Memory allocation failed", request->config.user_data);
        }
        cleanup_request(request);
        return ERR_OK;
    }
    
    snprintf(http_request, request_len,
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        request->path_copy, request->hostname_copy);
    
    // Send HTTP request
    err_t write_err = altcp_write(pcb, http_request, strlen(http_request), TCP_WRITE_FLAG_COPY);
    free(http_request);
    
    if (write_err != ERR_OK) {
        printf("Failed to send HTTP request: %d\n", write_err);
        if (request->config.error_callback) {
            request->config.error_callback("Failed to send request", request->config.user_data);
        }
        cleanup_request(request);
        return ERR_OK;
    }
    
    altcp_output(pcb);
    request->request_sent = true;
    printf("HTTP request sent\n");
    
    return ERR_OK;
}

static err_t altcp_recv_callback(void* arg, struct altcp_pcb* pcb, struct pbuf* buf, err_t err) {
    https_request_internal_t* request = (https_request_internal_t*)arg;
    
    if (err != ERR_OK || !request || !request->in_use) {
        if (buf) pbuf_free(buf);
        if (request && request->config.error_callback) {
            request->config.error_callback("Receive error", request->config.user_data);
        }
        if (request) cleanup_request(request);
        return ERR_OK;
    }
    
    if (!buf) {
        // Connection closed - process complete response
        printf("Connection closed, processing response\n");
        
        if (request->response_len > 0 && request->config.response_callback) {
            // Find start of HTTP body (after \r\n\r\n)
            char* body_start = NULL;
            if (request->response_buffer) {
                body_start = strstr(request->response_buffer, "\r\n\r\n");
                if (body_start) {
                    body_start += 4; // Skip \r\n\r\n
                    int body_len = request->response_len - (body_start - request->response_buffer);
                    request->config.response_callback(body_start, body_len, request->config.user_data);
                } else {
                    // No body separator found, send entire response
                    request->config.response_callback(request->response_buffer, request->response_len, request->config.user_data);
                }
            }
        }
        
        cleanup_request(request);
        return ERR_OK;
    }
    
    // Append received data to response buffer
    if (!request->response_buffer) {
        request->response_capacity = MAX_RESPONSE_SIZE;
        request->response_buffer = malloc(request->response_capacity);
        request->response_len = 0;
    }
    
    if (request->response_buffer && request->response_len + buf->len < request->response_capacity) {
        memcpy(request->response_buffer + request->response_len, buf->payload, buf->len);
        request->response_len += buf->len;
        request->response_buffer[request->response_len] = '\0';
    }
    
    // Acknowledge received data
    altcp_recved(pcb, buf->len);
    pbuf_free(buf);
    
    return ERR_OK;
}

static err_t altcp_sent_callback(void* arg, struct altcp_pcb* pcb, u16_t len) {
    printf("Data sent: %d bytes\n", len);
    return ERR_OK;
}

// DNS callback - renamed to avoid conflict with lwIP typedef
static void internal_dns_found_callback(const char* name, const ip_addr_t* ipaddr, void* callback_arg) {
    https_request_internal_t* request = (https_request_internal_t*)callback_arg;
    
    if (!ipaddr || !request || !request->in_use) {
        printf("DNS resolution failed for %s\n", name);
        if (request && request->config.error_callback) {
            request->config.error_callback("DNS resolution failed", request->config.user_data);
        }
        if (request) cleanup_request(request);
        return;
    }
    
    printf("DNS resolved for %s\n", name);
    
    // Create TLS configuration for client with no certificate validation (for now)
    // This allows connection to servers without requiring root CA certificates
    printf("Creating TLS client config for hostname: %s\n", request->hostname_copy);
    request->tls_config = altcp_tls_create_config_client(NULL, 0);
    if (!request->tls_config) {
        printf("Failed to create TLS config\n");
        if (request->config.error_callback) {
            request->config.error_callback("TLS config creation failed", request->config.user_data);
        }
        cleanup_request(request);
        return;
    }
    
    // Create ALTCP PCB
    request->pcb = altcp_tls_new(request->tls_config, IP_GET_TYPE(ipaddr));
    if (!request->pcb) {
        printf("Failed to create TLS connection\n");
        if (request->config.error_callback) {
            request->config.error_callback("TLS connection creation failed", request->config.user_data);
        }
        cleanup_request(request);
        return;
    }
    
    // Set up SNI (Server Name Indication) and additional TLS parameters for proper handshake
    void* tls_context = altcp_tls_context(request->pcb);
    if (tls_context) {
        mbedtls_ssl_context* ssl = (mbedtls_ssl_context*)tls_context;
        
        // Set SNI hostname
        int sni_ret = mbedtls_ssl_set_hostname(ssl, request->hostname_copy);
        if (sni_ret != 0) {
            printf("Failed to set SNI hostname: %d\n", sni_ret);
            // Continue anyway - some servers may still work without SNI
        } else {
            printf("SNI hostname set to: %s\n", request->hostname_copy);
        }
        
        // Disable certificate verification for now (common for embedded systems)
        mbedtls_ssl_conf_authmode(ssl->conf, MBEDTLS_SSL_VERIFY_NONE);
        printf("Certificate verification disabled for embedded compatibility\n");
        
        // Set minimum TLS version (try TLS 1.2 minimum)
        mbedtls_ssl_conf_min_version(ssl->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
        printf("Minimum TLS version set to 1.2\n");
        
    } else {
        printf("Warning: Could not get TLS context for SNI setup\n");
    }
    
    // Set up callbacks
    altcp_arg(request->pcb, request);
    altcp_err(request->pcb, altcp_error_callback);
    altcp_recv(request->pcb, altcp_recv_callback);
    altcp_sent(request->pcb, altcp_sent_callback);
    
    // Connect to server
    printf("Initiating TLS connection to %s:443 with SNI support\n", request->hostname_copy);
    err_t connect_err = altcp_connect(request->pcb, ipaddr, 443, altcp_connected_callback);
    if (connect_err != ERR_OK) {
        const char* error_str = get_altcp_error_string(connect_err);
        printf("Connection initiation failed: %d (%s)\n", connect_err, error_str);
        if (request->config.error_callback) {
            request->config.error_callback(error_str, request->config.user_data);
        }
        cleanup_request(request);
    } else {
        printf("TLS connection initiated successfully, waiting for handshake...\n");
    }
}

// Public API implementations

https_handle_t https_init(void) {
    https_service_internal_t* service = malloc(sizeof(https_service_internal_t));
    if (!service) {
        return NULL;
    }
    
    memset(service, 0, sizeof(https_service_internal_t));
    service->state = HTTPS_STATE_DISCONNECTED;
    
    printf("HTTPS service initialized\n");
    return (https_handle_t)service;
}

void https_cleanup(https_handle_t handle) {
    if (!handle) return;
    
    https_service_internal_t* service = (https_service_internal_t*)handle;
    
    // Cleanup all active requests
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (service->requests[i].in_use) {
            cleanup_request(&service->requests[i]);
        }
    }
    
    // Cleanup WiFi config copies
    if (service->wifi_ssid_copy) {
        free(service->wifi_ssid_copy);
    }
    if (service->wifi_password_copy) {
        free(service->wifi_password_copy);
    }
    
    free(service);
    printf("HTTPS service cleaned up\n");
}

bool https_wifi_init(https_handle_t handle, const https_wifi_config_t* config) {
    if (!handle || !config) return false;
    
    https_service_internal_t* service = (https_service_internal_t*)handle;
    
    printf("Initializing WiFi (CYW43)...\n");
    if (cyw43_arch_init()) {
        printf("ERROR: WiFi init failed!\n");
        return false;
    }
    cyw43_arch_enable_sta_mode();
    printf("WiFi hardware initialized\n");
    
    // Copy WiFi configuration
    service->wifi_ssid_copy = strdup(config->ssid);
    service->wifi_password_copy = strdup(config->password);
    service->wifi_config = *config;
    service->wifi_initialized = true;
    
    printf("Connecting to %s...\n", config->ssid);
    int result = cyw43_arch_wifi_connect_timeout_ms(
        config->ssid, 
        config->password, 
        CYW43_AUTH_WPA2_AES_PSK, 
        config->timeout_ms
    );
    
    if (result == 0) {
        printf("WiFi connected successfully!\n");
        service->wifi_connected = true;
        service->state = HTTPS_STATE_CONNECTED;
        
        // Wait for IP address
        while (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
            sleep_ms(100);
        }
        
        return true;
    } else {
        printf("WiFi connection failed with code %d\n", result);
        service->wifi_connected = false;
        service->state = HTTPS_STATE_ERROR;
        return false;
    }
}

https_state_t https_get_state(https_handle_t handle) {
    if (!handle) return HTTPS_STATE_ERROR;
    https_service_internal_t* service = (https_service_internal_t*)handle;
    return service->state;
}

bool https_is_connected(https_handle_t handle) {
    if (!handle) return false;
    https_service_internal_t* service = (https_service_internal_t*)handle;
    return service->wifi_connected;
}

int https_get_status(https_handle_t handle, char* buffer, int buffer_size) {
    if (!handle || !buffer || buffer_size <= 0) return 0;
    
    https_service_internal_t* service = (https_service_internal_t*)handle;
    
    const char* status_str;
    switch (service->state) {
        case HTTPS_STATE_DISCONNECTED: status_str = "Disconnected"; break;
        case HTTPS_STATE_CONNECTING: status_str = "Connecting"; break;
        case HTTPS_STATE_CONNECTED: 
            if (service->active_request_count > 0) {
                status_str = "Processing requests";
            } else {
                status_str = "Connected";
            }
            break;
        case HTTPS_STATE_REQUEST_PENDING: status_str = "Request pending"; break;
        case HTTPS_STATE_ERROR: status_str = "Error"; break;
        default: status_str = "Unknown"; break;
    }
    
    return snprintf(buffer, buffer_size, "%s", status_str);
}

https_request_t https_request(https_handle_t handle, const https_request_config_t* config) {
    if (!handle || !config || !config->hostname || !config->path) return NULL;
    
    https_service_internal_t* service = (https_service_internal_t*)handle;
    
    if (!service->wifi_connected) {
        if (config->error_callback) {
            config->error_callback("WiFi not connected", config->user_data);
        }
        return NULL;
    }
    
    https_request_internal_t* request = find_free_request_slot(service);
    if (!request) {
        if (config->error_callback) {
            config->error_callback("Too many pending requests", config->user_data);
        }
        return NULL;
    }
    
    // Initialize request
    request->config = *config;
    request->hostname_copy = strdup(config->hostname);
    request->path_copy = strdup(config->path);
    request->in_use = true;
    request->start_time = get_time_ms();
    service->active_request_count++;
    
    printf("Starting HTTPS request to %s%s\n", config->hostname, config->path);
    
    // Start DNS resolution
    ip_addr_t resolved_addr;
    err_t dns_result = dns_gethostbyname(config->hostname, &resolved_addr, internal_dns_found_callback, request);
    
    if (dns_result == ERR_OK) {
        // Already resolved
        internal_dns_found_callback(config->hostname, &resolved_addr, request);
    } else if (dns_result != ERR_INPROGRESS) {
        // DNS error
        printf("DNS error: %d\n", dns_result);
        if (config->error_callback) {
            config->error_callback("DNS error", config->user_data);
        }
        cleanup_request(request);
        service->active_request_count--;
        return NULL;
    }
    
    return (https_request_t)request;
}

bool https_cancel_request(https_handle_t handle, https_request_t request) {
    if (!handle || !request) return false;
    
    https_service_internal_t* service = (https_service_internal_t*)handle;
    https_request_internal_t* req = (https_request_internal_t*)request;
    
    // Verify request belongs to this service
    bool found = false;
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        if (&service->requests[i] == req) {
            found = true;
            break;
        }
    }
    
    if (found && req->in_use) {
        cleanup_request(req);
        service->active_request_count--;
        return true;
    }
    
    return false;
}

void https_poll(https_handle_t handle, int timeout_ms) {
    if (!handle) return;
    
    https_service_internal_t* service = (https_service_internal_t*)handle;
    
    if (service->wifi_connected) {
        cyw43_arch_poll();
    }
    
    // Check for timeouts
    uint32_t current_time = get_time_ms();
    for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
        https_request_internal_t* request = &service->requests[i];
        if (request->in_use) {
            int timeout = request->config.timeout_ms > 0 ? request->config.timeout_ms : DEFAULT_TIMEOUT_MS;
            if (current_time - request->start_time > timeout) {
                printf("Request timeout\n");
                if (request->config.error_callback) {
                    request->config.error_callback("Request timeout", request->config.user_data);
                }
                cleanup_request(request);
                service->active_request_count--;
            }
        }
    }
}

int https_get_pending_count(https_handle_t handle) {
    if (!handle) return 0;
    https_service_internal_t* service = (https_service_internal_t*)handle;
    return service->active_request_count;
}