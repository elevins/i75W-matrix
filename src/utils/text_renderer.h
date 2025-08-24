#pragma once

#include <string>
#include <cstdint>
#include "../core/common.hpp"

// UNIFIED TEXT RENDERING API
// Uses visual coordinates (like Python PIL reference) with automatic rotation handling

// Main unified text drawing function
void drawText(int x, int y, const std::string& text, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);

// Single character rendering (used internally by drawText)
void drawChar(int x, int y, char c, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);

// Color constants for convenience
#define COLOR_WHITE   255, 255, 255
#define COLOR_BLUE    100, 150, 255  // Low temp
#define COLOR_RED     255, 100, 100  // High temp
#define COLOR_YELLOW  255, 255, 0    // Accent

// Text measurement for layout
int measure_text_width(const std::string& text);

// Font properties (from bitmap font)
#define BITMAP_FONT_HEIGHT 5
#define BITMAP_FONT_SIZE 5

