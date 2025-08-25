#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "hardware/gpio.h"
#include "common.hpp"
#include "../apps/WeatherApp.hpp"
#include "../apps/StockApp.hpp"
#include "../apps/CryptoApp.hpp"
#include "../utils/text_renderer.h"
#include "../services/HTTPSService.hpp"


// GPIO pin definitions
#define ENCODER_A_PIN 19
#define ENCODER_B_PIN 21
#define ENCODER_SW_PIN 20
#define TILT_SWITCH_PIN 26

using namespace pimoroni;

// Global display objects (required by common.cpp)
Hub75 hub75(64, 32, nullptr, PANEL_GENERIC, false);
PicoGraphics_PenRGB888 graphics(64, 32, nullptr);

// App instances
WeatherApp weather_app;
StockApp stock_app;
CryptoApp crypto_app;

// HTTPS Service
HTTPSService https_service;

// Global state
AppType current_app = APP_WEATHER;
volatile bool tilt_active = false;
volatile bool encoder_button_clicked = false;

// Input state
static int encoder_pos = 0;
static int last_encoder_pos = 0;

// Polling state variables for controls
static uint32_t last_tilt_time = 0;
static uint32_t last_encoder_time = 0;
static uint32_t last_button_time = 0;
static int last_encoder_a = 0;
static int last_encoder_b = 0;
static int last_button_state = 1; // pulled up

// Poll all controls (no interrupts to avoid CYW43 conflicts)
void poll_controls() {
    uint32_t current_time = time_us_32();
    
    // Poll tilt switch with debouncing
    if (current_time - last_tilt_time > 50000) { // 50ms debounce
        bool new_tilt = !gpio_get(TILT_SWITCH_PIN); // Active low
        if (new_tilt != tilt_active) {
            tilt_active = new_tilt;
            printf("Tilt switch: %s\n", tilt_active ? "horizontal" : "vertical");
        }
        last_tilt_time = current_time;
    }
    
    // Poll encoder with debouncing
    if (current_time - last_encoder_time > 1000) { // 1ms debounce
        int a = gpio_get(ENCODER_A_PIN);
        int b = gpio_get(ENCODER_B_PIN);
        
        // Quadrature decoding
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
        if (button_state == 0 && last_button_state == 1) { // Falling edge
            encoder_button_clicked = true;
        }
        last_button_state = button_state;
        last_button_time = current_time;
    }
}

// Interrupt callback required function 
void __isr dma_complete() {
  hub75.dma_complete();
}

int main() {
    stdio_init_all();
    
    // Initialize display first
    printf("Initializing display...\n");
    hub75.start(dma_complete);
    
    // Show connecting screen
    graphics.set_pen(0, 0, 0);
    graphics.clear();
    drawText(3, 10, "Connecting WiFi...", COLOR_WHITE);
    hub75.update(&graphics);
    
    // Initialize HTTPS service (handles WiFi init internally)
    printf("Initializing HTTPS service...\n");
    if (https_service.initialize()) {
        printf("HTTPS service initialized successfully!\n");
        
        // Get IP address info
        while (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
            sleep_ms(100);
        }
        
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
        printf("HTTPS service ready\n");
        
        // Initialize all API data immediately on startup
        printf("=== INITIALIZING ALL APIs ON STARTUP ===\n");
        weather_app.initialize_api_data();
        stock_app.initialize_api_data();
        crypto_app.initialize_api_data();
        printf("=== API INITIALIZATION REQUESTS SENT ===\n");
        
    } else {
        printf("HTTPS service initialization failed\n");
        
        // Show error screen but continue
        graphics.set_pen(0, 0, 0);
        graphics.clear();
        drawText(3, 8, "WiFi Failed!", COLOR_RED);
        drawText(3, 18, "Using offline...", COLOR_WHITE);
        hub75.update(&graphics);
        sleep_ms(2000);
    }



    // Initialize GPIO for controls after WiFi is connected (polling, no interrupts)
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
    
    // Initialize polling state
    tilt_active = !gpio_get(TILT_SWITCH_PIN);
    last_encoder_a = gpio_get(ENCODER_A_PIN);
    last_encoder_b = gpio_get(ENCODER_B_PIN);
    last_button_state = gpio_get(ENCODER_SW_PIN);
    
    
    
    // Main loop with controls
    while (true) {
        // Poll HTTPS service (includes WiFi polling)
        https_service.poll();
        
        // Poll all controls (no interrupts)
        poll_controls();
        
        // Handle encoder app switching
        if (encoder_pos != last_encoder_pos) {
            int delta = encoder_pos - last_encoder_pos;
            
            if (delta > 0) {
                current_app = (AppType)((current_app + 1) % 3);
            } else if (delta < 0) {
                current_app = (AppType)((current_app + 2) % 3);
            }
            
            last_encoder_pos = encoder_pos;
            printf("Switched to app: %d\n", current_app);
        }
        
        // Handle button press
        if (encoder_button_clicked) {
            encoder_button_clicked = false;
            
            switch (current_app) {
                case APP_WEATHER:
                    weather_app.handle_button_press(tilt_active);
                    break;
                case APP_STOCKS:
                    stock_app.handle_button_press(tilt_active);
                    break;
                case APP_CRYPTO:
                    crypto_app.handle_button_press(tilt_active);
                    break;
            }
            printf("Button pressed for app: %d\n", current_app);
        }
        
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
    
    return 0;
}