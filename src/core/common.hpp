#pragma once

#include "pico/stdlib.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/interstate75/interstate75.hpp"
#include "libraries/pico_vector/pico_vector.hpp"
#include "libraries/pngdec/PNGdec.h"
#include <string>
#include <vector>

using namespace pimoroni;

// Display dimensions
extern const int DISPLAY_WIDTH;
extern const int DISPLAY_HEIGHT;

// Graphics and display objects
extern PicoGraphics_PenRGB888 graphics;
extern Hub75 hub75;
extern PicoVector picovector;

// WiFi status from main.cpp
extern bool wifi_connected;

// Application types
enum AppType {
    APP_WEATHER,
    APP_STOCKS,
    APP_CRYPTO
};

// Input status
enum class InputStatus {
    NONE,
    ENCODER_CW,
    ENCODER_CCW,
    BUTTON_PRESS
};

// Asset data structure
struct AssetData {
    std::string ticker;
    std::string name;
    std::string price;
    float change_24h;
};

// Weather data structure
struct WeatherData {
    std::string location;
    int current_temp;
    int min_temp;
    int max_temp;
    int humidity;
    int rain_chance;
    std::string icon_code;
    std::string sunrise;
    std::string sunset;
    std::string description;
    std::string day_name;
};

// Rotation modes for different orientations
enum class RotationMode {
    HORIZONTAL_UPSIDE_DOWN,  // 180° rotation for horizontal upside-down
    VERTICAL_CLOCKWISE       // 90° clockwise + upside-down for vertical
};

// Drawing functions
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, bool rotate = true);  // Legacy boolean interface
void draw_pixel_mode(int x, int y, uint8_t r, uint8_t g, uint8_t b, RotationMode rotation = RotationMode::HORIZONTAL_UPSIDE_DOWN);  // New mode interface
void draw_asset_logo(int x, int y, const std::string& ticker, uint8_t r, uint8_t g, uint8_t b, bool rotate = true);
void draw_weather_icon(int x, int y, const std::string& icon_code, bool rotate = true);
void set_custom_font_status(bool loaded);

// WiFi status indicator
void draw_wifi_status(int x, int y, bool rotate = true);

// Point helpers for rotation
Point rotate_180(int x, int y);
Point rotate_vertical(int x, int y);