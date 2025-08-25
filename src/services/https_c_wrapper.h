#ifndef HTTPS_C_WRAPPER_H
#define HTTPS_C_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Opaque handle types to completely hide lwIP/mbedTLS types
typedef void* https_handle_t;
typedef void* https_request_t;

// C-style callback function types
typedef void (*https_response_callback_t)(const char* response, int response_len, void* user_data);
typedef void (*https_error_callback_t)(const char* error_msg, void* user_data);
typedef void (*https_dns_callback_t)(const char* hostname, bool resolved, void* user_data);

// Connection states
typedef enum {
    HTTPS_STATE_DISCONNECTED,
    HTTPS_STATE_CONNECTING,
    HTTPS_STATE_CONNECTED,
    HTTPS_STATE_REQUEST_PENDING,
    HTTPS_STATE_ERROR
} https_state_t;

// Request structure for C interface
typedef struct {
    const char* hostname;
    const char* path;
    https_response_callback_t response_callback;
    https_error_callback_t error_callback;
    void* user_data;
    int timeout_ms;
} https_request_config_t;

// WiFi configuration
typedef struct {
    const char* ssid;
    const char* password;
    int timeout_ms;
} https_wifi_config_t;

/**
 * Initialize the HTTPS service
 * @return Handle to HTTPS service instance, or NULL on failure
 */
https_handle_t https_init(void);

/**
 * Cleanup and destroy HTTPS service
 * @param handle HTTPS service handle
 */
void https_cleanup(https_handle_t handle);

/**
 * Initialize WiFi connection
 * @param handle HTTPS service handle
 * @param config WiFi configuration
 * @return true on success, false on failure
 */
bool https_wifi_init(https_handle_t handle, const https_wifi_config_t* config);

/**
 * Get current connection state
 * @param handle HTTPS service handle
 * @return Current state
 */
https_state_t https_get_state(https_handle_t handle);

/**
 * Check if WiFi is connected
 * @param handle HTTPS service handle
 * @return true if connected, false otherwise
 */
bool https_is_connected(https_handle_t handle);

/**
 * Get status string
 * @param handle HTTPS service handle
 * @param buffer Buffer to write status string
 * @param buffer_size Size of buffer
 * @return Number of bytes written to buffer
 */
int https_get_status(https_handle_t handle, char* buffer, int buffer_size);

/**
 * Queue an HTTPS request
 * @param handle HTTPS service handle
 * @param config Request configuration
 * @return Request handle, or NULL on failure
 */
https_request_t https_request(https_handle_t handle, const https_request_config_t* config);

/**
 * Cancel a pending request
 * @param handle HTTPS service handle
 * @param request Request handle
 * @return true if cancelled, false if not found or already completed
 */
bool https_cancel_request(https_handle_t handle, https_request_t request);

/**
 * Poll the HTTPS service (call regularly in main loop)
 * @param handle HTTPS service handle
 * @param timeout_ms Maximum time to spend processing (0 for non-blocking)
 */
void https_poll(https_handle_t handle, int timeout_ms);

/**
 * Get number of pending requests
 * @param handle HTTPS service handle
 * @return Number of requests in queue
 */
int https_get_pending_count(https_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // HTTPS_C_WRAPPER_H