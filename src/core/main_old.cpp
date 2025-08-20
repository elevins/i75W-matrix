#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <string>
#include <vector>

#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/interstate75/interstate75.hpp"

using namespace pimoroni;

// GPIO pin definitions
#define ENCODER_A_PIN 19
#define ENCODER_B_PIN 21
#define ENCODER_SW_PIN 20
#define TILT_SWITCH_PIN 26

// Display size for 64x32 HUB75 panel
Hub75 hub75(64, 32, nullptr, PANEL_GENERIC, false);

// PicoGraphics surface for 64x32 display
PicoGraphics_PenRGB888 graphics(64, 32, nullptr);

// Display dimensions
const int width = 64;
const int height = 32;

// Application states
enum AppType {
    APP_WEATHER,
    APP_STOCKS,
    APP_CRYPTO
};

// Global variables for input handling
volatile int encoder_pos = 0;
volatile bool encoder_pressed = false;
volatile bool encoder_button_clicked = false;
volatile bool tilt_active = false;

// Application state
AppType current_app = APP_WEATHER;
int app_sub_state = 0;

// Quadrature encoder state tracking
static int last_a = 0;
static int last_b = 0;
static int encoder_state = 0;

// Quadrature lookup table for state transitions
// Each state transition maps to direction: 0=no change, 1=CW, -1=CCW
static const int8_t quadrature_table[16] = {
    0, -1,  1,  0,   // State 0 (00)
    1,  0,  0, -1,   // State 1 (01)
   -1,  0,  0,  1,   // State 2 (10)
    0,  1, -1,  0    // State 3 (11)
};

// Debouncing state variables (keeping button and tilt debouncing)
static uint32_t last_button_time = 0;
static uint32_t last_tilt_time = 0;
static bool last_tilt_state = false;
static bool last_button_state = false;
static bool stable_tilt_state = false;

// Debouncing thresholds
#define BUTTON_DEBOUNCE_MS 50
#define TILT_DEBOUNCE_MS 100

// GPIO interrupt handler for rotary encoder (minimal, debouncing done in main loop)
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

// Function to rotate coordinates 180 degrees for upside-down display
Point rotate_180(int x, int y) {
    return Point(width - 1 - x, height - 1 - y);
}

// Safe pixel drawing with optional 180-degree rotation
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, bool rotate = true) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        graphics.set_pen(r, g, b);
        if (rotate) {
            Point rotated = rotate_180(x, y);
            graphics.pixel(rotated);
        } else {
            graphics.pixel(Point(x, y));
        }
    }
}

// Text drawing function (simple bitmap characters)
void draw_char(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b, bool rotate = true) {
    // Simple 3x5 font for basic characters
    uint8_t font[95][5] = {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // space
        {0x04, 0x04, 0x04, 0x00, 0x04}, // !
        {0x0A, 0x0A, 0x00, 0x00, 0x00}, // "
        {0x0A, 0x1F, 0x0A, 0x1F, 0x0A}, // #
        {0x0E, 0x14, 0x0E, 0x05, 0x0E}, // $
        {0x18, 0x19, 0x02, 0x04, 0x13}, // %
        {0x08, 0x14, 0x08, 0x15, 0x0E}, // &
        {0x04, 0x04, 0x00, 0x00, 0x00}, // '
        {0x02, 0x04, 0x04, 0x04, 0x02}, // (
        {0x08, 0x04, 0x04, 0x04, 0x08}, // )
        {0x00, 0x0A, 0x04, 0x0A, 0x00}, // *
        {0x00, 0x04, 0x0E, 0x04, 0x00}, // +
        {0x00, 0x00, 0x00, 0x04, 0x08}, // ,
        {0x00, 0x00, 0x0E, 0x00, 0x00}, // -
        {0x00, 0x00, 0x00, 0x00, 0x04}, // .
        {0x01, 0x02, 0x04, 0x08, 0x10}, // /
        {0x0E, 0x11, 0x11, 0x11, 0x0E}, // 0
        {0x04, 0x0C, 0x04, 0x04, 0x0E}, // 1
        {0x0E, 0x01, 0x0E, 0x10, 0x1F}, // 2
        {0x1F, 0x02, 0x06, 0x01, 0x1E}, // 3
        {0x02, 0x06, 0x0A, 0x1F, 0x02}, // 4
        {0x1F, 0x10, 0x1E, 0x01, 0x1E}, // 5
        {0x06, 0x08, 0x1E, 0x11, 0x0E}, // 6
        {0x1F, 0x01, 0x02, 0x04, 0x08}, // 7
        {0x0E, 0x11, 0x0E, 0x11, 0x0E}, // 8
        {0x0E, 0x11, 0x0F, 0x02, 0x0C}, // 9
        {0x00, 0x04, 0x00, 0x04, 0x00}, // :
        {0x00, 0x04, 0x00, 0x04, 0x08}, // ;
        {0x02, 0x04, 0x08, 0x04, 0x02}, // <
        {0x00, 0x0E, 0x00, 0x0E, 0x00}, // =
        {0x08, 0x04, 0x02, 0x04, 0x08}, // >
        {0x0E, 0x01, 0x06, 0x00, 0x04}, // ?
        {0x0E, 0x11, 0x15, 0x15, 0x0E}, // @
        {0x0E, 0x11, 0x1F, 0x11, 0x11}, // A
        {0x1E, 0x11, 0x1E, 0x11, 0x1E}, // B
        {0x0E, 0x11, 0x10, 0x11, 0x0E}, // C
        {0x1C, 0x12, 0x11, 0x12, 0x1C}, // D
        {0x1F, 0x10, 0x1E, 0x10, 0x1F}, // E
        {0x1F, 0x10, 0x1E, 0x10, 0x10}, // F
        {0x0E, 0x11, 0x17, 0x11, 0x0F}, // G
        {0x11, 0x11, 0x1F, 0x11, 0x11}, // H
        {0x0E, 0x04, 0x04, 0x04, 0x0E}, // I
        {0x07, 0x02, 0x02, 0x12, 0x0C}, // J
        {0x11, 0x12, 0x1C, 0x12, 0x11}, // K
        {0x10, 0x10, 0x10, 0x10, 0x1F}, // L
        {0x11, 0x1B, 0x15, 0x11, 0x11}, // M
        {0x11, 0x19, 0x15, 0x13, 0x11}, // N
        {0x0E, 0x11, 0x11, 0x11, 0x0E}, // O
        {0x1E, 0x11, 0x1E, 0x10, 0x10}, // P
        {0x0E, 0x11, 0x15, 0x13, 0x0F}, // Q
        {0x1E, 0x11, 0x1E, 0x12, 0x11}, // R
        {0x0F, 0x10, 0x0E, 0x01, 0x1E}, // S
        {0x1F, 0x04, 0x04, 0x04, 0x04}, // T
        {0x11, 0x11, 0x11, 0x11, 0x0E}, // U
        {0x11, 0x11, 0x11, 0x0A, 0x04}, // V
        {0x11, 0x11, 0x15, 0x1B, 0x11}, // W
        {0x11, 0x0A, 0x04, 0x0A, 0x11}, // X
        {0x11, 0x0A, 0x04, 0x04, 0x04}, // Y
        {0x1F, 0x02, 0x04, 0x08, 0x1F}, // Z
    };
    
    if (c >= 32 && c <= 126) {
        int index = c - 32;
        for (int row = 0; row < 5; row++) {
            uint8_t line = font[index][row];
            for (int col = 0; col < 5; col++) {
                if (line & (1 << (4 - col))) {
                    draw_pixel(x + col, y + row, r, g, b, rotate);
                }
            }
        }
    }
}

void draw_string(int x, int y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, bool rotate = true) {
    for (size_t i = 0; i < text.length(); i++) {
        draw_char(x + i * 6, y, text[i], r, g, b, rotate);
    }
}

// Draw simple asset logos (8x8 pixel icons)
void draw_asset_logo(int x, int y, const std::string& ticker, uint8_t r, uint8_t g, uint8_t b, bool rotate = true) {
    if (ticker == "BTC") {
        // Bitcoin logo (simplified ₿)
        uint8_t bitcoin_logo[8] = {0x1C, 0x22, 0x7E, 0x22, 0x22, 0x7E, 0x22, 0x1C};
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (bitcoin_logo[row] & (1 << (7 - col))) {
                    draw_pixel(x + col, y + row, r, g, b, rotate);
                }
            }
        }
    } else if (ticker == "ETH") {
        // Ethereum logo (diamond shape)
        uint8_t eth_logo[8] = {0x18, 0x3C, 0x7E, 0xFF, 0x7E, 0x3C, 0x7E, 0x42};
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (eth_logo[row] & (1 << (7 - col))) {
                    draw_pixel(x + col, y + row, r, g, b, rotate);
                }
            }
        }
    } else if (ticker == "AAPL") {
        // Apple logo (simplified apple)
        uint8_t apple_logo[8] = {0x0C, 0x1E, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x02};
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (apple_logo[row] & (1 << (7 - col))) {
                    draw_pixel(x + col, y + row, r, g, b, rotate);
                }
            }
        }
    } else if (ticker == "TSLA") {
        // Tesla logo (simplified T)
        uint8_t tesla_logo[8] = {0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (tesla_logo[row] & (1 << (7 - col))) {
                    draw_pixel(x + col, y + row, r, g, b, rotate);
                }
            }
        }
    } else {
        // Default: Draw ticker as text
        draw_string(x, y, ticker.substr(0, 3), r, g, b, rotate);
    }
}

// Weather app data
std::vector<std::string> weather_data = {"SUNNY", "72F", "CLOUDY", "68F", "RAINY", "65F"};
std::vector<std::string> weather_forecast = {"MON 75F", "TUE 73F", "WED 70F"};

// Asset data structures
struct AssetData {
    std::string ticker;
    std::string name;
    std::string price;
    float change_24h;  // Percentage change (positive/negative)
};

// Stock app data  
std::vector<AssetData> stock_assets = {
    {"AAPL", "Apple", "150.25", 2.1f},
    {"GOOGL", "Google", "2800.50", -1.2f},
    {"MSFT", "Microsoft", "375.80", 0.8f},
    {"TSLA", "Tesla", "245.30", -3.4f}
};

// Crypto app data
std::vector<AssetData> crypto_assets = {
    {"BTC", "Bitcoin", "45.2K", 5.7f},
    {"ETH", "Ethereum", "3.2K", -2.1f},
    {"ADA", "Cardano", "0.45", 12.3f},
    {"SOL", "Solana", "105", -4.8f}
};

void draw_weather_app(bool is_horizontal) {
    bool show_forecast = (app_sub_state == 1);
    bool rotate = true; // Always rotate 180° for proper orientation
    
    if (is_horizontal) {
        if (show_forecast) {
            draw_string(2, 2, "FORECAST", 255, 255, 0, rotate);
            for (int i = 0; i < 3 && i < weather_forecast.size(); i++) {
                draw_string(2, 10 + i * 7, weather_forecast[i], 255, 255, 255, rotate);
            }
        } else {
            draw_string(2, 2, "WEATHER", 255, 255, 0, rotate);
            draw_string(2, 12, weather_data[0], 0, 255, 0, rotate);
            draw_string(2, 22, weather_data[1], 255, 255, 255, rotate);
        }
    } else {
        if (show_forecast) {
            draw_string(2, 2, "FORE", 255, 255, 0, rotate);
            draw_string(2, 10, "CAST", 255, 255, 0, rotate);
            draw_string(35, 2, weather_forecast[0].substr(0, 3), 255, 255, 255, rotate);
            draw_string(35, 10, weather_forecast[0].substr(4), 255, 255, 255, rotate);
        } else {
            draw_string(2, 2, "WEAT", 255, 255, 0, rotate);
            draw_string(2, 10, "HER", 255, 255, 0, rotate);
            draw_string(2, 20, weather_data[0].substr(0, 4), 0, 255, 0, rotate);
            draw_string(35, 20, weather_data[1], 255, 255, 255, rotate);
        }
    }
}

void draw_stocks_app(bool is_horizontal) {
    bool rotate = true; // Always rotate 180° for proper orientation
    
    if (is_horizontal) {
        // Horizontal: Single asset with large logo and details (stockTicker style)
        int asset_idx = app_sub_state % stock_assets.size();
        const AssetData& asset = stock_assets[asset_idx];
        
        // Large asset logo (left side)
        draw_asset_logo(2, 8, asset.ticker, 255, 255, 255, rotate);
        
        // Asset name and ticker (top right)
        draw_string(12, 2, asset.name, 255, 255, 0, rotate);
        draw_string(12, 10, asset.ticker, 200, 200, 200, rotate);
        
        // Current price (large, middle right)
        draw_string(12, 18, "$" + asset.price, 255, 255, 255, rotate);
        
        // 24h change with color coding (bottom right)
        uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
        uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
        uint8_t change_b = 0;
        
        std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
        draw_string(12, 26, change_str, change_r, change_g, change_b, rotate);
        
    } else {
        // Vertical: Static list of top 4 stocks
        draw_string(2, 1, "STOCKS", 255, 255, 0, rotate);
        
        for (int i = 0; i < 4 && i < stock_assets.size(); i++) {
            const AssetData& asset = stock_assets[i];
            int y_pos = 8 + i * 6;
            
            // Small logo + ticker
            draw_string(2, y_pos, asset.ticker, 200, 200, 200, rotate);
            
            // Price
            draw_string(24, y_pos, "$" + asset.price.substr(0, 6), 255, 255, 255, rotate);
            
            // 24h change with color
            uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
            uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
            uint8_t change_b = 0;
            
            std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
            draw_string(45, y_pos, change_str, change_r, change_g, change_b, rotate);
        }
    }
}

void draw_crypto_app(bool is_horizontal) {
    bool rotate = true; // Always rotate 180° for proper orientation
    
    if (is_horizontal) {
        // Horizontal: Single asset with large logo and details (stockTicker style)
        int asset_idx = app_sub_state % crypto_assets.size();
        const AssetData& asset = crypto_assets[asset_idx];
        
        // Large asset logo (left side)
        draw_asset_logo(2, 8, asset.ticker, 255, 165, 0, rotate);
        
        // Asset name and ticker (top right)
        draw_string(12, 2, asset.name, 255, 255, 0, rotate);
        draw_string(12, 10, asset.ticker, 200, 200, 200, rotate);
        
        // Current price (large, middle right)
        draw_string(12, 18, "$" + asset.price, 255, 255, 255, rotate);
        
        // 24h change with color coding (bottom right)
        uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
        uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
        uint8_t change_b = 0;
        
        std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
        draw_string(12, 26, change_str, change_r, change_g, change_b, rotate);
        
    } else {
        // Vertical: Static list of top 4 cryptos
        draw_string(2, 1, "CRYPTO", 255, 255, 0, rotate);
        
        for (int i = 0; i < 4 && i < crypto_assets.size(); i++) {
            const AssetData& asset = crypto_assets[i];
            int y_pos = 8 + i * 6;
            
            // Small logo + ticker
            draw_string(2, y_pos, asset.ticker, 255, 165, 0, rotate);
            
            // Price
            draw_string(20, y_pos, "$" + asset.price, 255, 255, 255, rotate);
            
            // 24h change with color
            uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
            uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
            uint8_t change_b = 0;
            
            std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
            draw_string(40, y_pos, change_str, change_r, change_g, change_b, rotate);
        }
    }
}

// Interrupt callback required function 
void __isr dma_complete() {
  hub75.dma_complete();
}


int main() {
    stdio_init_all();

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

    // Main variables
    static int last_encoder_pos = 0;
    bool is_horizontal = true;

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
                app_sub_state = 0;
            } else if (delta < 0) {
                current_app = (AppType)((current_app + 2) % 3);
                app_sub_state = 0;
            }
            
            last_encoder_pos = encoder_pos;
        }

        // Handle button clicks (sub-state changes)
        if (encoder_button_clicked) {
            encoder_button_clicked = false;
            
            switch (current_app) {
                case APP_WEATHER:
                    // Weather: Always toggle current/forecast
                    app_sub_state = (app_sub_state + 1) % 2;
                    break;
                case APP_STOCKS:
                    // Stocks: Only cycle assets when horizontal, disabled when vertical
                    if (is_horizontal) {
                        app_sub_state = (app_sub_state + 1) % stock_assets.size();
                    }
                    break;
                case APP_CRYPTO:
                    // Crypto: Only cycle assets when horizontal, disabled when vertical
                    if (is_horizontal) {
                        app_sub_state = (app_sub_state + 1) % crypto_assets.size();
                    }
                    break;
            }
        }

        // Draw current app
        switch (current_app) {
            case APP_WEATHER:
                draw_weather_app(is_horizontal);
                break;
            case APP_STOCKS:
                draw_stocks_app(is_horizontal);
                break;
            case APP_CRYPTO:
                draw_crypto_app(is_horizontal);
                break;
        }

        // Draw app indicator dots at bottom
        bool rotate = true; // Always rotate 180° for proper orientation
        for (int i = 0; i < 3; i++) {
            uint8_t r = (i == current_app) ? 255 : 64;
            uint8_t g = (i == current_app) ? 255 : 64;
            uint8_t b = (i == current_app) ? 255 : 64;
            
            int dot_x = 25 + i * 8;
            int dot_y = 29;
            draw_pixel(dot_x, dot_y, r, g, b, rotate);
            draw_pixel(dot_x + 1, dot_y, r, g, b, rotate);
            draw_pixel(dot_x, dot_y + 1, r, g, b, rotate);
            draw_pixel(dot_x + 1, dot_y + 1, r, g, b, rotate);
        }

        // Update display
        hub75.update(&graphics);
        sleep_ms(50);
    }

    return 0;
}