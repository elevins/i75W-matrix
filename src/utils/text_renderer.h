#pragma once

#include <string>
#include <cstdint>
#include "../core/common.hpp"

// Bitmap text rendering functions
// Uses visual coordinates (like Python PIL reference) with automatic rotation handling

// Core bitmap text rendering function
void draw_text_bitmap(int visual_x, int visual_y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, bool rotate = true);

// Single character rendering  
void draw_char_bitmap(int visual_x, int visual_y, char c, uint8_t r, uint8_t g, uint8_t b, bool rotate = true);

// New rotation mode core functions
void draw_text_bitmap_mode(int visual_x, int visual_y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);
void draw_char_bitmap_mode(int visual_x, int visual_y, char c, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);

// Convenience functions with predefined colors for weather app (legacy boolean interface)
void draw_text_white(int x, int y, const std::string& text, bool rotate = true);
void draw_text_blue(int x, int y, const std::string& text, bool rotate = true);   // Low temp
void draw_text_red(int x, int y, const std::string& text, bool rotate = true);    // High temp  
void draw_text_yellow(int x, int y, const std::string& text, bool rotate = true); // Accent

// New rotation mode versions for different orientations
void draw_text_white_mode(int x, int y, const std::string& text, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);
void draw_text_blue_mode(int x, int y, const std::string& text, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);
void draw_text_red_mode(int x, int y, const std::string& text, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);
void draw_text_yellow_mode(int x, int y, const std::string& text, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);

// Text measurement for layout
int measure_text_width(const std::string& text);

// Font properties (from bitmap font)
#define BITMAP_FONT_HEIGHT 5
#define BITMAP_FONT_SIZE 5