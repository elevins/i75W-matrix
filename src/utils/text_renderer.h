#pragma once

#include <string>
#include <cstdint>

// Bitmap text rendering functions
// Uses visual coordinates (like Python PIL reference) with automatic rotation handling

// Core bitmap text rendering function
void draw_text_bitmap(int visual_x, int visual_y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, bool rotate = true);

// Single character rendering
void draw_char_bitmap(int visual_x, int visual_y, char c, uint8_t r, uint8_t g, uint8_t b, bool rotate = true);

// Convenience functions with predefined colors for weather app
void draw_text_white(int x, int y, const std::string& text, bool rotate = true);
void draw_text_blue(int x, int y, const std::string& text, bool rotate = true);   // Low temp
void draw_text_red(int x, int y, const std::string& text, bool rotate = true);    // High temp  
void draw_text_yellow(int x, int y, const std::string& text, bool rotate = true); // Accent

// Text measurement for layout
int measure_text_width(const std::string& text);

// Font properties (from bitmap font)
#define BITMAP_FONT_HEIGHT 5
#define BITMAP_FONT_SIZE 5