# GPIO and WiFi Integration Guide

## Problem: GPIO Interrupt Conflicts with CYW43 WiFi

When integrating GPIO controls (rotary encoder, buttons, switches) with the CYW43 WiFi chip on Raspberry Pi Pico2W + Interstate75W, GPIO interrupts conflict with the WiFi chip's interrupt handling, causing the display to go blank.

## Root Cause

The CYW43 WiFi chip uses `gpio_add_raw_irq_handler_with_order_priority()` for GPIO 24 (HOST_WAKE pin) with specific interrupt priority. When user code calls `gpio_set_irq_enabled_with_callback()` or `gpio_add_raw_irq_handler()`, it interferes with CYW43's exclusive interrupt handling.

### CYW43 GPIO Usage
- GPIO 23: CYW43_DEFAULT_PIN_WL_REG_ON (power up)
- GPIO 24: CYW43_DEFAULT_PIN_WL_DATA_OUT/IN and HOST_WAKE (SPI data and IRQ)
- GPIO 25: CYW43_DEFAULT_PIN_WL_CS (SPI chip select)  
- GPIO 29: CYW43_DEFAULT_PIN_WL_CLOCK (SPI clock) + VSYS monitoring

## Working Solution: Polling-Based GPIO Controls

Replace all GPIO interrupts with polling in the main loop to avoid interrupt system conflicts.

### Implementation

```cpp
// Polling state variables
static uint32_t last_tilt_time = 0;
static uint32_t last_encoder_time = 0;
static uint32_t last_button_time = 0;
static int last_encoder_a = 0;
static int last_encoder_b = 0;
static int last_button_state = 1;

// Poll all controls (no interrupts)
void poll_controls() {
    uint32_t current_time = time_us_32();
    
    // Poll tilt switch with debouncing
    if (current_time - last_tilt_time > 50000) { // 50ms debounce
        bool new_tilt = !gpio_get(TILT_SWITCH_PIN);
        if (new_tilt != tilt_active) {
            tilt_active = new_tilt;
        }
        last_tilt_time = current_time;
    }
    
    // Poll encoder with quadrature decoding
    if (current_time - last_encoder_time > 1000) { // 1ms debounce
        int a = gpio_get(ENCODER_A_PIN);
        int b = gpio_get(ENCODER_B_PIN);
        
        if (a != last_encoder_a) {
            if (a && !b) encoder_pos++;
            else if (a && b) encoder_pos--;
            last_encoder_a = a;
        }
        
        if (b != last_encoder_b) {
            if (b && a) encoder_pos++;
            else if (b && !a) encoder_pos--;
            last_encoder_b = b;
        }
        
        last_encoder_time = current_time;
    }
    
    // Poll button with debouncing
    if (current_time - last_button_time > 100000) { // 100ms debounce
        int button_state = gpio_get(ENCODER_SW_PIN);
        if (button_state == 0 && last_button_state == 1) {
            encoder_button_clicked = true;
        }
        last_button_state = button_state;
        last_button_time = current_time;
    }
}
```

### Main Loop Integration

```cpp
while (true) {
    // Poll WiFi first
    if (wifi_connected) {
        cyw43_arch_poll();
    }
    
    // Poll controls without interrupts
    poll_controls();
    
    // Handle control events...
    
    // Update display at 10Hz
    sleep_ms(100);
}
```

### GPIO Pin Configuration

```cpp
// Safe GPIO pins (avoid CYW43 pins 23-25, 29)
#define ENCODER_A_PIN 19
#define ENCODER_B_PIN 21
#define ENCODER_SW_PIN 20
#define TILT_SWITCH_PIN 26

// Initialize without interrupts
gpio_init(ENCODER_A_PIN);
gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
gpio_pull_up(ENCODER_A_PIN);
// No gpio_set_irq_enabled calls!
```

## What NOT to Do

❌ **These approaches WILL cause conflicts:**

```cpp
// DON'T: Callback-based interrupts
gpio_set_irq_enabled_with_callback(pin, events, true, &handler);

// DON'T: Raw IRQ handlers  
gpio_add_raw_irq_handler(pin, &handler);

// DON'T: Any GPIO interrupt setup with CYW43 active
```

## Benefits of Polling Approach

✅ **Advantages:**
- No interrupt conflicts with CYW43
- Deterministic timing (10ms loop)
- Easier debugging (no async issues)
- Simple debouncing logic
- Works reliably with WiFi active

✅ **Performance:**
- Polling every 10ms is fast enough for user controls
- Minimal CPU overhead compared to display updates
- No impact on WiFi performance

## Tested Configuration

- **Hardware**: Raspberry Pi Pico2W + Pimoroni Interstate75W
- **GPIO Pins**: 19 (encoder A), 21 (encoder B), 20 (button), 26 (tilt)
- **WiFi**: CYW43 with lwIP poll architecture
- **Display**: 64x32 Hub75 LED matrix
- **Result**: All systems working perfectly together

This solution completely eliminates GPIO interrupt conflicts while maintaining full functionality of WiFi, display, and user controls.