#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "common.hpp"
#include "../apps/WeatherApp.hpp"
#include "../apps/StockApp.hpp"
#include "../apps/CryptoApp.hpp"

// GPIO pin definitions
#define ENCODER_A_PIN 19
#define ENCODER_B_PIN 21
#define ENCODER_SW_PIN 20
#define TILT_SWITCH_PIN 26

// Global graphics and display objects
Hub75 hub75(64, 32, nullptr, PANEL_GENERIC, false);
PicoGraphics_PenRGB888 graphics(64, 32, nullptr);
PicoVector picovector(&graphics);

// Global input state
volatile int encoder_pos = 0;
volatile bool encoder_pressed = false;
volatile bool encoder_button_clicked = false;
volatile bool tilt_active = false;

// Application state
AppType current_app = APP_WEATHER;

// Quadrature encoder state tracking
static int last_a = 0;
static int last_b = 0;
static int encoder_state = 0;

// Quadrature lookup table for state transitions
static const int8_t quadrature_table[16] = {
    0, -1,  1,  0,   // State 0 (00)
    1,  0,  0, -1,   // State 1 (01)
   -1,  0,  0,  1,   // State 2 (10)
    0,  1, -1,  0    // State 3 (11)
};

// Debouncing state variables
static uint32_t last_button_time = 0;
static uint32_t last_tilt_time = 0;
static bool last_tilt_state = false;
static bool last_button_state = false;
static bool stable_tilt_state = false;

// Debouncing thresholds
#define BUTTON_DEBOUNCE_MS 50
#define TILT_DEBOUNCE_MS 100

// App instances
WeatherApp weather_app;
StockApp stock_app;
CryptoApp crypto_app;

// GPIO interrupt handler
void encoder_interrupt(uint gpio, uint32_t events) {
    // Just wake up the main loop, actual processing done there with debouncing
}

// Quadrature encoder processing
bool process_quadrature_encoder() {
    int a = gpio_get(ENCODER_A_PIN);
    int b = gpio_get(ENCODER_B_PIN);
    
    // Only process if either pin changed
    if (a != last_a || b != last_b) {
        // Build new state from current pin readings
        int new_state = (a << 1) | b;
        
        // Look up direction from state transition table
        int table_index = (encoder_state << 2) | new_state;
        int8_t direction = quadrature_table[table_index];
        
        if (direction != 0) {
            encoder_pos += direction;
        }
        
        // Update state for next iteration
        encoder_state = new_state;
        last_a = a;
        last_b = b;
        
        return (direction != 0);
    }
    
    return false;
}

bool debounce_button() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool current_button = !gpio_get(ENCODER_SW_PIN); // Active low
    
    if (current_button != last_button_state) {
        last_button_time = current_time;
        last_button_state = current_button;
        return false;
    }
    
    if (current_time - last_button_time > BUTTON_DEBOUNCE_MS) {
        if (current_button && !encoder_pressed) {
            encoder_pressed = true;
            encoder_button_clicked = true;
            return true;
        } else if (!current_button && encoder_pressed) {
            encoder_pressed = false;
        }
    }
    
    return false;
}

bool debounce_tilt() {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    bool current_tilt = !gpio_get(TILT_SWITCH_PIN); // Active low, adjust if needed
    
    if (current_tilt != last_tilt_state) {
        last_tilt_time = current_time;
        last_tilt_state = current_tilt;
        return false;
    }
    
    if (current_time - last_tilt_time > TILT_DEBOUNCE_MS) {
        if (stable_tilt_state != current_tilt) {
            stable_tilt_state = current_tilt;
            tilt_active = stable_tilt_state;
            return true;
        }
    }
    
    return false;
}

// Interrupt callback required function 
void __isr dma_complete() {
  hub75.dma_complete();
}

int main() {
    stdio_init_all();
    
    printf("\n=== i75 Weather Display Starting ===\n");

    // Initialize GPIO pins
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

    // Set up GPIO interrupts
    gpio_set_irq_enabled_with_callback(ENCODER_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &encoder_interrupt);
    gpio_set_irq_enabled(ENCODER_B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENCODER_SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(TILT_SWITCH_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Initialize quadrature encoder state
    last_a = gpio_get(ENCODER_A_PIN);
    last_b = gpio_get(ENCODER_B_PIN);
    encoder_state = (last_a << 1) | last_b;
    
    // Initialize debouncing state
    last_tilt_state = !gpio_get(TILT_SWITCH_PIN);
    stable_tilt_state = last_tilt_state;
    tilt_active = stable_tilt_state;
    last_button_state = !gpio_get(ENCODER_SW_PIN);

    // Start hub75 driver
    hub75.start(dma_complete);

    // Using built-in bitmap fonts - no setup needed
    set_custom_font_status(true); // Mark as "working" since bitmap fonts always work
    
    printf("Display initialized\n");

    // Main variables
    static int last_encoder_pos = 0;
    bool is_horizontal = true;
    
    printf("Entering main loop...\n");

    while(true) {
        
        // Clear screen
        graphics.set_pen(0, 0, 0);
        graphics.clear();

        // Process inputs
        process_quadrature_encoder();
        debounce_button();
        debounce_tilt();

        // Determine orientation (tilt_active = true means horizontal, false means vertical)
        is_horizontal = tilt_active;

        // Handle encoder position changes (app switching)
        if (encoder_pos != last_encoder_pos) {
            int delta = encoder_pos - last_encoder_pos;
            
            if (delta > 0) {
                current_app = (AppType)((current_app + 1) % 3);
                // Reset sub-states when switching apps
                weather_app.reset_state();
                stock_app.reset_state();
                crypto_app.reset_state();
            } else if (delta < 0) {
                current_app = (AppType)((current_app + 2) % 3);
                // Reset sub-states when switching apps
                weather_app.reset_state();
                stock_app.reset_state();
                crypto_app.reset_state();
            }
            
            last_encoder_pos = encoder_pos;
        }

        // Handle button clicks using app-specific handlers
        if (encoder_button_clicked) {
            encoder_button_clicked = false;
            
            switch (current_app) {
                case APP_WEATHER:
                    weather_app.handle_button_press(is_horizontal);
                    break;
                case APP_STOCKS:
                    stock_app.handle_button_press(is_horizontal);
                    break;
                case APP_CRYPTO:
                    crypto_app.handle_button_press(is_horizontal);
                    break;
            }
        }

        // Draw current app using modular approach
        switch (current_app) {
            case APP_WEATHER:
                weather_app.draw(is_horizontal);
                break;
            case APP_STOCKS:
                stock_app.draw(is_horizontal);
                break;
            case APP_CRYPTO:
                crypto_app.draw(is_horizontal);
                break;
        }


        // Update display
        hub75.update(&graphics);
        sleep_ms(50);
    }

    return 0;
}