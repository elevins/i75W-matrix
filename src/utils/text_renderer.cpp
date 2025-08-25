#include "text_renderer.h"
#include "../core/common.hpp"
#include "tiny_bitmap.h"

// =============================================================================
// UNIFIED TEXT RENDERING API
// =============================================================================

// Single character rendering using bitmap font with rotation
void drawChar(int visual_x, int visual_y, char c, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation) {
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
                
                // Use draw_pixel_mode function which handles rotation
                draw_pixel_mode(pixel_x, pixel_y, r, g, b, rotation);
            }
        }
    }
}

// Main unified text drawing function
//TODO add text alignment left/right to this for prettiness 
void drawText(int x, int y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation) {
    int current_x = x;
    
    for (char c : text) {
        const BitmapChar* char_bitmap = get_char_bitmap(c);
        if (char_bitmap) {
            // Draw the character
            drawChar(current_x, y, c, r, g, b, rotation);
            
            // Advance cursor by character width + 1px spacing
            current_x += char_bitmap->width + 1;
        }
    }
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