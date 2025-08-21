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

## Main Loop Integration

```cpp
while (true) {
    // WiFi polling must happen regularly
    if (wifi_connected) {
        cyw43_arch_poll();
    }
    
    // Application logic
    poll_controls();
    update_display();
    
    // 10Hz update rate works well
    sleep_ms(100);
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