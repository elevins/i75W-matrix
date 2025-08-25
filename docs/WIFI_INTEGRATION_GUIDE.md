# WiFi Integration Guide for Pico2W + Interstate75W

## Complete Working Solution

This guide documents the successful integration of CYW43 WiFi with Hub75 LED matrix display on Raspberry Pi Pico2W + Pimoroni Interstate75W hardware.

## Critical Requirements

### CMakeLists.txt Configuration
```cmake
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_BOARD pico2_w)

# CRITICAL: Use poll-based CYW43 architecture
target_link_libraries(${NAME}
    hub75
    pico_stdlib
    pico_multicore
    pimoroni_i2c
    pico_graphics
    pico_vector
    pngdec
    hardware_pwm
    pico_cyw43_arch_lwip_poll  # NOT pico_wireless!
)
```

### Required Header Files
```cpp
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "hardware/gpio.h"
```

## Initialization Sequence - CRITICAL ORDER

The initialization order is absolutely critical for success:

```cpp
int main() {
    stdio_init_all();
    
    // 1. Initialize CYW43 WiFi chip FIRST
    printf("Initializing WiFi (CYW43)...\n");
    if (cyw43_arch_init()) {
        printf("ERROR: WiFi init failed!\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("WiFi initialized successfully\n");
    
    // 2. Initialize Hub75 display AFTER WiFi
    printf("Initializing display...\n");
    hub75.start(dma_complete);
    
    // 3. Show connection screens using proper rendering
    graphics.set_pen(0, 0, 0);
    graphics.clear();
    draw_text_white(3, 10, "Connecting WiFi...", true);
    hub75.update(&graphics);
    
    // 4. Connect to WiFi network
    printf("Connecting to network...\n");
    int result = cyw43_arch_wifi_connect_timeout_ms(
        "SSID", "PASSWORD", CYW43_AUTH_WPA2_AES_PSK, 30000
    );
    
    if (result == 0) {
        printf("WiFi connected successfully!\n");
        wifi_connected = true;
        
        // Wait for IP assignment
        while (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
            sleep_ms(100);
        }
        
        // Show success screen
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        draw_text_white(3, 8, "WiFi Connected!", true);
        draw_text_white(3, 18, "Starting app...", true);
        hub75.update(&graphics);
        sleep_ms(2000);
    }
    
    // 5. Initialize GPIO controls LAST (polling only)
    // ... GPIO setup code ...
    
    // 6. Main loop
    while (true) {
        if (wifi_connected) {
            cyw43_arch_poll();  // Poll WiFi
        }
        
        poll_controls();        // Poll GPIO controls
        
        // ... application logic ...
        
        sleep_ms(100);          // 10Hz update rate
    }
}
```

## Key Technical Details

### WiFi Architecture Choice
- **Use**: `pico_cyw43_arch_lwip_poll` - Poll-based, single-threaded
- **Don't use**: `pico_wireless` - ESP32SPI library (wrong chip)
- **Don't use**: `pico_cyw43_arch_lwip_threadsafe_background` - Threading conflicts

### CYW43 vs Hub75 Compatibility
The CYW43 WiFi chip and Hub75 display driver can coexist because:
- CYW43 uses dedicated SPI pins (23, 24, 25, 29)
- Hub75 uses different GPIO pins for LED matrix control
- Poll-based architecture avoids threading conflicts
- Proper initialization order prevents GPIO conflicts

### IP Address Retrieval
```cpp
// Get IP address after connection
if (netif_default != NULL) {
    const ip4_addr_t *ip = netif_ip4_addr(netif_default);
    uint32_t ip_addr = ip4_addr_get_u32(ip);
    printf("IP Address: %u.%u.%u.%u\n", 
           ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, 
           (ip_addr >> 16) & 0xFF, (ip_addr >> 24) & 0xFF);
}
```

## Main Loop Integration with HTTPS Service

```cpp
// Global HTTPS service instance
HTTPSService https_service;

while (true) {
    // 1. HTTPS service polling (includes WiFi polling internally)
    https_service.poll();
    
    // 2. Application control polling
    poll_controls();
    
    // 3. Handle encoder app switching and button presses
    handle_user_input();
    
    // 4. Update display based on current app
    update_display();
    
    // 10Hz update rate works well for all systems
    sleep_ms(100);
}
```

### **HTTPS Service Integration Details**
The `HTTPSService` class handles all networking internally:
```cpp
void HTTPSService::poll() {
    // Poll WiFi connection status
    cyw43_arch_poll();
    
    // Poll HTTPS requests (DNS, TLS, HTTP state machines)
    https_poll(https_handle);
    
    // Handle connection state changes
    update_connection_status();
}
```

### **C/C++ Header Isolation Pattern**
To avoid conflicts between PNGdec (`#define local static`) and lwIP headers:

**C Wrapper (`https_c_wrapper.h/c`):**
```c
// https_c_wrapper.h - Pure C interface, no lwIP headers exposed
typedef struct https_service_internal* https_handle_t;
typedef struct https_request_config {
    const char* hostname;
    const char* path;
    void (*success_callback)(const char* response, void* user_data);
    void (*error_callback)(const char* error, void* user_data);
    void* user_data;
} https_request_config_t;

// Pure C functions
https_handle_t https_init(void);
int https_wifi_init(https_handle_t handle, const char* ssid, const char* password);
int https_request(https_handle_t handle, const https_request_config_t* config);
void https_poll(https_handle_t handle);
```

**C++ Wrapper (`HTTPSService.hpp`):**
```cpp
// HTTPSService.hpp - Clean C++ interface, no lwIP headers
#include <string>
#include <functional>

class HTTPSService {
public:
    using SuccessCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    bool initialize();
    void request(const std::string& hostname, const std::string& path, 
                SuccessCallback success_cb, ErrorCallback error_cb);
    void poll();
    
private:
    void* https_handle;  // Opaque handle to C implementation
};
```

### **Complete Initialization Sequence**
```cpp
int main() {
    stdio_init_all();
    
    // 1. Initialize display first (prevents GPIO conflicts)
    printf("Initializing display...\n");
    hub75.start(dma_complete);
    
    // 2. Show connecting screen immediately  
    graphics.set_pen(0, 0, 0);
    graphics.clear();
    drawText(3, 10, "Connecting WiFi...", COLOR_WHITE);
    hub75.update(&graphics);
    
    // 3. Initialize HTTPS service (handles WiFi internally)
    printf("Initializing HTTPS service...\n");
    if (https_service.initialize()) {
        printf("HTTPS service initialized successfully!\n");
        
        // Wait for IP assignment and show status
        while (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
            sleep_ms(100);
        }
        
        // Display IP address info
        if (netif_default != NULL) {
            const ip4_addr_t *ip = netif_ip4_addr(netif_default);
            uint32_t ip_addr = ip4_addr_get_u32(ip);
            printf("IP Address: %u.%u.%u.%u\n", 
                   ip_addr & 0xFF, (ip_addr >> 8) & 0xFF, 
                   (ip_addr >> 16) & 0xFF, (ip_addr >> 24) & 0xFF);
        }
        
        // Show success screen
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        drawText(3, 8, "HTTPS Ready!", COLOR_WHITE);
        drawText(3, 18, "Starting app...", COLOR_WHITE);
        hub75.update(&graphics);
        sleep_ms(2000);
        
        // 4. Initialize all API data immediately on startup
        printf("=== INITIALIZING ALL APIs ON STARTUP ===\n");
        weather_app.initialize_api_data();
        stock_app.initialize_api_data();
        crypto_app.initialize_api_data();
        printf("=== API INITIALIZATION REQUESTS SENT ===\n");
        
    } else {
        printf("HTTPS service initialization failed\n");
        // Show error but continue with offline mode
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        drawText(3, 8, "WiFi Failed!", COLOR_RED);
        drawText(3, 18, "Using offline...", COLOR_WHITE);
        hub75.update(&graphics);
        sleep_ms(2000);
    }

    // 5. Initialize GPIO controls last (polling only, no interrupts)
    printf("Initializing controls (polling mode)...\n");
    gpio_init(ENCODER_A_PIN);
    gpio_init(ENCODER_B_PIN);
    gpio_init(ENCODER_SW_PIN);
    gpio_init(TILT_SWITCH_PIN);
    
    gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
    gpio_set_dir(ENCODER_B_PIN, GPIO_IN);
    gpio_set_dir(ENCODER_SW_PIN, GPIO_IN);
    gpio_set_dir(TILT_SWITCH_PIN, GPIO_IN);
    
    gpio_pull_up(ENCODER_A_PIN);
    gpio_pull_up(ENCODER_B_PIN);
    gpio_pull_up(ENCODER_SW_PIN);
    gpio_pull_up(TILT_SWITCH_PIN);
    
    // 6. Main loop with integrated HTTPS polling
    while (true) {
        // Poll HTTPS service (includes WiFi polling internally)
        https_service.poll();
        
        // Poll all controls (no interrupts to avoid CYW43 conflicts)
        poll_controls();
        
        // Handle encoder app switching
        handle_app_switching();
        
        // Handle button press
        handle_button_press();
        
        // Clear screen and draw current app
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        
        // Draw current app based on tilt (true = horizontal, false = vertical)
        switch (current_app) {
            case APP_WEATHER:
                weather_app.draw(tilt_active);
                break;
            case APP_STOCKS:
                stock_app.draw(tilt_active);
                break;
            case APP_CRYPTO:
                crypto_app.draw(tilt_active);
                break;
        }
        
        // Update display
        hub75.update(&graphics);
        
        // Update at 10Hz
        sleep_ms(100);
    }
}
```

## Common Pitfalls to Avoid

❌ **Wrong library**: Using `pico_wireless` instead of `pico_cyw43_arch_lwip_poll`
❌ **Wrong order**: Initializing Hub75 before CYW43
❌ **GPIO interrupts**: Using interrupts for controls (conflicts with CYW43)
❌ **Missing polling**: Forgetting `cyw43_arch_poll()` in main loop
❌ **Threading**: Attempting to use threaded WiFi architecture

## Verification Steps

1. **Build succeeds** without linker errors
2. **Console output** shows:
   ```
   Initializing WiFi (CYW43)...
   WiFi initialized successfully
   Initializing display...
   Connecting to [SSID]...
   WiFi connected successfully!
   IP Address: 192.168.1.xxx
   ```
3. **Display shows** connection screens and then normal app
4. **Controls work** without breaking WiFi or display

## Hardware Requirements

- Raspberry Pi Pico2W (not regular Pico2)
- Pimoroni Interstate75W with RM2 module
- 64x32 Hub75 LED matrix
- Stable 5V power supply for LED matrix

This configuration has been tested and verified to work reliably with WiFi connectivity, LED matrix display, and GPIO controls all functioning simultaneously.