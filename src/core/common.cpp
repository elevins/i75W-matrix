#include "common.hpp"
#include "libraries/bitmap_fonts/font8_data.hpp"
#include "libraries/bitmap_fonts/font6_data.hpp"
#include "../assets/weather/weather_01d_png.h"
#include "../assets/weather/weather_02d_png.h"
#include "../assets/weather/weather_03d_png.h"
#include "../assets/weather/weather_04d_png.h"
#include "../assets/weather/weather_09d_png.h"
#include "../assets/weather/weather_10d_png.h"
#include "../assets/weather/weather_11d_png.h"
#include "../assets/weather/weather_13d_png.h"
#include "../assets/weather/weather_50d_png.h"

// Display constants
const int DISPLAY_WIDTH = 64;
const int DISPLAY_HEIGHT = 32;

// Function to rotate coordinates 180 degrees for upside-down display
Point rotate_180(int x, int y) {
    return Point(DISPLAY_WIDTH - 1 - x, DISPLAY_HEIGHT - 1 - y);
}

// Safe pixel drawing with optional 180-degree rotation
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
    if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        graphics.set_pen(r, g, b);
        if (rotate) {
            Point rotated = rotate_180(x, y);
            graphics.pixel(rotated);
        } else {
            graphics.pixel(Point(x, y));
        }
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

// PNG decode callback function for drawing pixels to display
void PNGDraw(PNGDRAW *pDraw) {
    uint8_t *s = (uint8_t *)pDraw->pPixels;
    
    // Get coordinates from user data (passed as x,y offset)
    int *coords = (int *)pDraw->pUser;
    int base_x = coords[0];
    int base_y = coords[1];
    bool rotate = coords[2] != 0;
    
    // Convert PNG pixels and draw them
    for (int x = 0; x < pDraw->iWidth; x++) {
        uint8_t r, g, b, a = 255;
        
        if (pDraw->iBpp == 32) { // RGBA
            r = s[x * 4];
            g = s[x * 4 + 1]; 
            b = s[x * 4 + 2];
            a = s[x * 4 + 3];
        } else if (pDraw->iBpp == 24) { // RGB
            r = s[x * 3];
            g = s[x * 3 + 1];
            b = s[x * 3 + 2];
        } else if (pDraw->iBpp == 8) { // Indexed
            uint8_t idx = s[x];
            if (pDraw->pPalette && idx < 256) {
                r = pDraw->pPalette[idx * 3];
                g = pDraw->pPalette[idx * 3 + 1];
                b = pDraw->pPalette[idx * 3 + 2];
            } else {
                r = g = b = idx; // Grayscale fallback
            }
        } else {
            continue; // Unsupported format
        }
        
        // Skip transparent pixels
        if (a < 128) continue;
        
        // Draw pixel at offset position
        draw_pixel(base_x + x, base_y + pDraw->y, r, g, b, rotate);
    }
}

// Function to draw weather icon from PNG data
void draw_weather_icon(int x, int y, const std::string& icon_code, bool rotate) {
    PNG png;
    int rc;
    
    // Select appropriate PNG data based on icon code
    const unsigned char *png_data = nullptr;
    unsigned int png_len = 0;
    
    if (icon_code == "01d" || icon_code == "01n") {
        png_data = weather_01d_png_data;
        png_len = weather_01d_png_len;
    } else if (icon_code == "02d" || icon_code == "02n") {
        png_data = weather_02d_png_data;
        png_len = weather_02d_png_len;
    } else if (icon_code == "03d" || icon_code == "03n") {
        png_data = weather_03d_png_data;
        png_len = weather_03d_png_len;
    } else if (icon_code == "04d" || icon_code == "04n") {
        png_data = weather_04d_png_data;
        png_len = weather_04d_png_len;
    } else if (icon_code == "09d" || icon_code == "09n") {
        png_data = weather_09d_png_data;
        png_len = weather_09d_png_len;
    } else if (icon_code == "10d" || icon_code == "10n") {
        png_data = weather_10d_png_data;
        png_len = weather_10d_png_len;
    } else if (icon_code == "11d" || icon_code == "11n") {
        png_data = weather_11d_png_data;
        png_len = weather_11d_png_len;
    } else if (icon_code == "13d" || icon_code == "13n") {
        png_data = weather_13d_png_data;
        png_len = weather_13d_png_len;
    } else if (icon_code == "50d" || icon_code == "50n") {
        png_data = weather_50d_png_data;
        png_len = weather_50d_png_len;
    }
    
    if (png_data == nullptr) {
        // Fallback: draw a simple icon placeholder
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                if ((i + j) % 4 == 0) {
                    draw_pixel(x + i, y + j, 255, 255, 0, rotate);
                }
            }
        }
        return;
    }
    
    // Pass coordinates to callback via user data
    static int coords[3];  // Make static to ensure memory stays valid during decode
    coords[0] = x;
    coords[1] = y; 
    coords[2] = rotate ? 1 : 0;
    
    // Initialize PNG decoder using openRAM for embedded data
    rc = png.openRAM((uint8_t *)png_data, png_len, PNGDraw);
    if (rc == PNG_SUCCESS) {
        png.decode((void *)coords, 0);
        png.close();
    }
}