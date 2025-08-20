#include "common.hpp"
#include "libraries/bitmap_fonts/font8_data.hpp"
#include "libraries/bitmap_fonts/font6_data.hpp"
#include "../assets/weather/weather_01d_png_new.h"
#include "../assets/weather/weather_01n_png_new.h"
#include "../assets/weather/weather_02d_png_new.h"
#include "../assets/weather/weather_02n_png_new.h"
#include "../assets/weather/weather_03d_png_new.h"
#include "../assets/weather/weather_03n_png_new.h"
#include "../assets/weather/weather_04d_png_new.h"
#include "../assets/weather/weather_04n_png_new.h"
#include "../assets/weather/weather_09d_png_new.h"
#include "../assets/weather/weather_09n_png_new.h"
#include "../assets/weather/weather_10d_png_new.h"
#include "../assets/weather/weather_10n_png_new.h"
#include "../assets/weather/weather_11d_png_new.h"
#include "../assets/weather/weather_11n_png_new.h"
#include "../assets/weather/weather_13d_png_new.h"
#include "../assets/weather/weather_13n_png_new.h"
#include "../assets/weather/weather_50d_png_new.h"
#include "../assets/weather/weather_50n_png_new.h"

// Display constants
const int DISPLAY_WIDTH = 64;
const int DISPLAY_HEIGHT = 32;

// Function to rotate coordinates 180 degrees for upside-down display
Point rotate_180(int x, int y) {
    return Point(DISPLAY_WIDTH - 1 - x, DISPLAY_HEIGHT - 1 - y);
}

// Function to rotate coordinates for vertical orientation (90° clockwise + upside-down)
Point rotate_vertical(int x, int y) {
    // For vertical mode: 32x64 logical becomes 64x32 physical
    // 90° clockwise: (x,y) -> (y, 31-x) for 32-wide logical screen
    // Then upside-down: (x,y) -> (63-x, 31-y) for 64x32 physical
    int rotated_x = y;
    int rotated_y = 31 - x;  // 31 = DISPLAY_HEIGHT - 1
    return Point(DISPLAY_WIDTH - 1 - rotated_x, DISPLAY_HEIGHT - 1 - rotated_y);
}

// Legacy boolean interface for backward compatibility
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
    RotationMode mode = rotate ? RotationMode::HORIZONTAL_UPSIDE_DOWN : RotationMode::HORIZONTAL_UPSIDE_DOWN;
    draw_pixel_mode(x, y, r, g, b, mode);
}

// New rotation mode interface
void draw_pixel_mode(int x, int y, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation) {
    // For vertical mode, check against logical 32x64 bounds
    int max_x = (rotation == RotationMode::VERTICAL_CLOCKWISE) ? 32 : DISPLAY_WIDTH;
    int max_y = (rotation == RotationMode::VERTICAL_CLOCKWISE) ? 64 : DISPLAY_HEIGHT;
    
    if (x >= 0 && x < max_x && y >= 0 && y < max_y) {
        graphics.set_pen(r, g, b);
        
        Point final_point;
        if (rotation == RotationMode::HORIZONTAL_UPSIDE_DOWN) {
            final_point = rotate_180(x, y);
        } else if (rotation == RotationMode::VERTICAL_CLOCKWISE) {
            final_point = rotate_vertical(x, y);
        } else {
            final_point = Point(x, y); // No rotation
        }
        
        graphics.pixel(final_point);
    }
}

// Global flag to track font loading status
static bool font_loaded_successfully = false;

// Text drawing using built-in Pimoroni bitmap fonts
void draw_string(int x, int y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
    // Green pixel = using working bitmap fonts
    graphics.set_pen(0, 255, 0);
    graphics.pixel(Point(0, 31));
    
    // Set text color
    graphics.set_pen(r, g, b);
    
    // Use font6 - smallest available font for compact display
    graphics.set_font(&font6);
    
    // Use built-in bitmap text - simple and reliable
    if (rotate) {
        // For 180-degree rotation, calculate rotated coordinates
        Point rotated = rotate_180(x, y);
        graphics.text(text, rotated, -1);
    } else {
        graphics.text(text, Point(x, y), -1);
    }
}

// Function to set custom font status
void set_custom_font_status(bool loaded) {
    font_loaded_successfully = loaded;
}

// Legacy function for backward compatibility - now just calls draw_string
void draw_char(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
    std::string single_char;
    single_char += c;
    draw_string(x, y, single_char, r, g, b, rotate);
}

// Draw simple asset logos (8x8 pixel icons)
void draw_asset_logo(int x, int y, const std::string& ticker, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
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

void PNGDraw(PNGDRAW *pDraw) {
    uint8_t *s = (uint8_t *)pDraw->pPixels;
    int *coords = (int *)pDraw->pUser;
    int base_x = coords[0];
    int base_y = coords[1];
    bool rotate = coords[2] != 0;
    
    for (int x = 0; x < pDraw->iWidth; x++) {
        uint8_t r, g, b;
        
        if (pDraw->iBpp == 8) { // 8-bit indexed color
            uint8_t idx = s[x];
            if (pDraw->pPalette && idx < 256) {
                r = pDraw->pPalette[idx * 3];
                g = pDraw->pPalette[idx * 3 + 1];
                b = pDraw->pPalette[idx * 3 + 2];
            } else {
                continue;
            }
        } else if (pDraw->iBpp == 24) { // RGB
            r = s[x * 3];
            g = s[x * 3 + 1];
            b = s[x * 3 + 2];
        } else if (pDraw->iBpp == 32) { // RGBA
            r = s[x * 4];
            g = s[x * 4 + 1]; 
            b = s[x * 4 + 2];
            uint8_t a = s[x * 4 + 3];
            if (a < 128) continue;
        } else {
            continue;
        }
        
        // Skip black pixels (often transparent in indexed images)
        if (r == 0 && g == 0 && b == 0) continue;
        
        draw_pixel(base_x + x, base_y + pDraw->y, r, g, b, rotate);
    }
}

void draw_weather_icon(int x, int y, const std::string& icon_code, bool rotate) {
    PNG png;
    
    // Select PNG data based on icon code
    const unsigned char *png_data = nullptr;
    unsigned int png_len = 0;
    
    if (icon_code == "01d") {
        png_data = __01d_png;
        png_len = __01d_png_len;
    } else if (icon_code == "01n") {
        png_data = __01n_png;
        png_len = __01n_png_len;
    } else if (icon_code == "02d") {
        png_data = __02d_png;
        png_len = __02d_png_len;
    } else if (icon_code == "02n") {
        png_data = __02n_png;
        png_len = __02n_png_len;
    } else if (icon_code == "03d") {
        png_data = __03d_png;
        png_len = __03d_png_len;
    } else if (icon_code == "03n") {
        png_data = __03n_png;
        png_len = __03n_png_len;
    } else if (icon_code == "04d") {
        png_data = __04d_png;
        png_len = __04d_png_len;
    } else if (icon_code == "04n") {
        png_data = __04n_png;
        png_len = __04n_png_len;
    } else if (icon_code == "09d") {
        png_data = __09d_png;
        png_len = __09d_png_len;
    } else if (icon_code == "09n") {
        png_data = __09n_png;
        png_len = __09n_png_len;
    } else if (icon_code == "10d") {
        png_data = __10d_png;
        png_len = __10d_png_len;
    } else if (icon_code == "10n") {
        png_data = __10n_png;
        png_len = __10n_png_len;
    } else if (icon_code == "11d") {
        png_data = __11d_png;
        png_len = __11d_png_len;
    } else if (icon_code == "11n") {
        png_data = __11n_png;
        png_len = __11n_png_len;
    } else if (icon_code == "13d") {
        png_data = __13d_png;
        png_len = __13d_png_len;
    } else if (icon_code == "13n") {
        png_data = __13n_png;
        png_len = __13n_png_len;
    } else if (icon_code == "50d") {
        png_data = __50d_png;
        png_len = __50d_png_len;
    } else if (icon_code == "50n") {
        png_data = __50n_png;
        png_len = __50n_png_len;
    }
    
    // Set up coordinates for callback
    static int coords[3];
    coords[0] = x;
    coords[1] = y;
    coords[2] = rotate ? 1 : 0;
    
    if (png_data != nullptr) {
        // Try to decode embedded PNG data
        int rc = png.openRAM((uint8_t *)png_data, png_len, PNGDraw);
        if (rc == PNG_SUCCESS) {
            png.decode((void *)coords, 0);
            png.close();
            return; // Success, exit early
        }
    }
    
    // Fallback: draw simple bitmap icon
    if (icon_code == "01d" || icon_code == "01n") {
        // Clear sky - sun icon
        for (int i = 6; i <= 9; i++) {
            for (int j = 6; j <= 9; j++) {
                draw_pixel(x + i, y + j, 255, 255, 0, rotate);
            }
        }
        // Sun rays
        draw_pixel(x + 7, y + 2, 255, 255, 0, rotate);
        draw_pixel(x + 7, y + 13, 255, 255, 0, rotate);
        draw_pixel(x + 2, y + 7, 255, 255, 0, rotate);
        draw_pixel(x + 13, y + 7, 255, 255, 0, rotate);
    } else {
        // Default cloud for other weather
        for (int i = 3; i <= 12; i++) {
            for (int j = 5; j <= 10; j++) {
                draw_pixel(x + i, y + j, 150, 150, 150, rotate);
            }
        }
    }
}