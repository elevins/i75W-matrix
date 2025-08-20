#include "text_renderer.h"
#include "../core/common.hpp"
#include "tiny_bitmap.h"

// Render single character using bitmap font with rotation
void draw_char_bitmap(int visual_x, int visual_y, char c, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
    const BitmapChar* char_bitmap = get_char_bitmap(c);
    if (!char_bitmap || !char_bitmap->data) {
        return; // Skip invalid characters
    }
    
    // Draw each pixel of the character bitmap
    for (int y = 0; y < char_bitmap->height; y++) {
        uint8_t row_data = char_bitmap->data[y];
        
        for (int x = 0; x < char_bitmap->width; x++) {
            // Check if pixel should be drawn (bit test)
            if (row_data & (1 << (7 - x))) {
                int pixel_x = visual_x + x;
                int pixel_y = visual_y + y;
                
                // Use existing draw_pixel function which handles rotation
                draw_pixel(pixel_x, pixel_y, r, g, b, rotate);
            }
        }
    }
}

// Render text string using bitmap font with Python-style coordinates
void draw_text_bitmap(int visual_x, int visual_y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, bool rotate) {
    int current_x = visual_x;
    
    for (char c : text) {
        const BitmapChar* char_bitmap = get_char_bitmap(c);
        if (char_bitmap) {
            // Draw the character
            draw_char_bitmap(current_x, visual_y, c, r, g, b, rotate);
            
            // Advance cursor by character width + 1px spacing
            current_x += char_bitmap->width + 1;
        }
    }
}

// Convenience functions for weather app colors
void draw_text_white(int x, int y, const std::string& text, bool rotate) {
    draw_text_bitmap(x, y, text, 255, 255, 255, rotate);
}

void draw_text_blue(int x, int y, const std::string& text, bool rotate) {
    draw_text_bitmap(x, y, text, 100, 150, 255, rotate);
}

void draw_text_red(int x, int y, const std::string& text, bool rotate) {
    draw_text_bitmap(x, y, text, 255, 100, 100, rotate);
}

void draw_text_yellow(int x, int y, const std::string& text, bool rotate) {
    draw_text_bitmap(x, y, text, 255, 255, 0, rotate);
}

// New rotation mode text rendering functions
void draw_char_bitmap_mode(int visual_x, int visual_y, char c, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation) {
    const BitmapChar* char_bitmap = get_char_bitmap(c);
    if (!char_bitmap || !char_bitmap->data) {
        return; // Skip invalid characters
    }
    
    // Draw each pixel of the character bitmap
    for (int y = 0; y < char_bitmap->height; y++) {
        uint8_t row_data = char_bitmap->data[y];
        
        for (int x = 0; x < char_bitmap->width; x++) {
            // Check if pixel should be drawn (bit test)
            if (row_data & (1 << (7 - x))) {
                int pixel_x = visual_x + x;
                int pixel_y = visual_y + y;
                
                // Use new draw_pixel_mode function which handles rotation
                draw_pixel_mode(pixel_x, pixel_y, r, g, b, rotation);
            }
        }
    }
}

void draw_text_bitmap_mode(int visual_x, int visual_y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation) {
    int current_x = visual_x;
    
    for (char c : text) {
        const BitmapChar* char_bitmap = get_char_bitmap(c);
        if (char_bitmap) {
            // Draw the character
            draw_char_bitmap_mode(current_x, visual_y, c, r, g, b, rotation);
            
            // Advance cursor by character width + 1px spacing
            current_x += char_bitmap->width + 1;
        }
    }
}

// New rotation mode convenience functions for weather app colors
void draw_text_white_mode(int x, int y, const std::string& text, RotationMode rotation) {
    draw_text_bitmap_mode(x, y, text, 255, 255, 255, rotation);
}

void draw_text_blue_mode(int x, int y, const std::string& text, RotationMode rotation) {
    draw_text_bitmap_mode(x, y, text, 100, 150, 255, rotation);
}

void draw_text_red_mode(int x, int y, const std::string& text, RotationMode rotation) {
    draw_text_bitmap_mode(x, y, text, 255, 100, 100, rotation);
}

void draw_text_yellow_mode(int x, int y, const std::string& text, RotationMode rotation) {
    draw_text_bitmap_mode(x, y, text, 255, 255, 0, rotation);
}

// Calculate text width for layout purposes
int measure_text_width(const std::string& text) {
    int total_width = 0;
    
    for (char c : text) {
        const BitmapChar* char_bitmap = get_char_bitmap(c);
        if (char_bitmap) {
            total_width += char_bitmap->width + 1; // width + spacing
        }
    }
    
    // Remove trailing spacing
    if (total_width > 0) {
        total_width -= 1;
    }
    
    return total_width;
}