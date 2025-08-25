#include "WeatherApp.hpp"
#include "../utils/text_renderer.h"
#include "../core/common.hpp"
#include "../services/HTTPSService.hpp"
#include <sstream>
#include <iomanip>
#include <cstdio>

WeatherApp::WeatherApp() : api_data_loading(false), api_data_available(false), startup_api_completed(false) {
    initialize_mock_data();
    load_weather_icons();
}

void WeatherApp::initialize_mock_data() {
    // Initial data for NYC - will be replaced by API calls
    current_weather = {
        "New York",     // location
        72,             // current_temp
        65,             // min_temp
        78,             // max_temp
        60,             // humidity
        15,             // rain_chance
        "01d",          // icon_code (clear day)
        "6:45",         // sunrise
        "7:32",         // sunset
        "Clear skies",  // description
        "Today"         // day_name
    };
    
    // Mock forecast data - 5 days for weekly view
    forecast_data = {
        {"NYC", 75, 68, 82, 55, 10, "02d", "6:46", "7:31", "Partly cloudy", "MON"},
        {"NYC", 73, 66, 80, 65, 25, "10d", "6:47", "7:30", "Light rain", "TUE"},
        {"NYC", 71, 64, 77, 70, 40, "04d", "6:48", "7:29", "Cloudy", "WED"},
        {"NYC", 69, 62, 75, 45, 5, "01d", "6:49", "7:28", "Clear", "THU"},
        {"NYC", 74, 67, 81, 50, 15, "03d", "6:50", "7:27", "Scattered clouds", "FRI"}
    };
}

void WeatherApp::load_weather_icons() {
    // For now, we'll use placeholder logic
    // TODO: Implement PNG loading for weather icons
    // This will require adding PNG decoding capabilities
}

void WeatherApp::request_weather_data() {
    if (!api_data_loading && https_service.is_connected()) {
        api_data_loading = true;
        printf("Requesting weather data from Open-Meteo API...\n");
        
        // Open-Meteo API - Free, no API key required
        // NYC coordinates: 40.7128°N 74.0060°W
        std::string path = "/v1/forecast?latitude=40.7128&longitude=-74.0060&current_weather=true&hourly=temperature_2m,relative_humidity_2m,precipitation_probability&daily=temperature_2m_max,temperature_2m_min&timezone=America/New_York&temperature_unit=fahrenheit";
        
        https_service.request("api.open-meteo.com", path,
            [this](const std::string& response) {
                this->parse_weather_response(response);
            },
            [this](const std::string& error) {
                this->on_weather_error(error);
            },
            15000  // 15 second timeout
        );
    }
}

void WeatherApp::parse_weather_response(const std::string& response) {
    printf("=== WEATHER API RESPONSE ===\n");
    printf("Response length: %zu bytes\n", response.length());
    printf("Response content: %s\n", response.c_str());
    printf("=== END RESPONSE ===\n");
    
    // Mark as successful - detailed JSON parsing can be added later
    api_data_loading = false;
    api_data_available = true;
    startup_api_completed = true;
    
    // TODO: Parse Open-Meteo JSON response and update current_weather structure
    // Open-Meteo format: {"current_weather":{"temperature":72.5,"windspeed":3.2,...}}
    printf("Weather API response received successfully!\n");
}

void WeatherApp::on_weather_error(const std::string& error) {
    printf("=== WEATHER API ERROR ===\n");
    printf("Error: %s\n", error.c_str());
    printf("=== END ERROR ===\n");
    api_data_loading = false;
    // Keep using mock data
}

void WeatherApp::initialize_api_data() {
    printf("WeatherApp: Initializing API data on startup...\n");
    request_weather_data();
}

void WeatherApp::draw(bool is_horizontal) {
    bool rotate = true; // Always rotate 180° for proper orientation
    
    // Only do periodic refresh after startup API is completed
    if (startup_api_completed) {
        static uint32_t last_request_time = 0;
        uint32_t current_time = time_us_32() / 1000000; // Convert to seconds
        if (current_time - last_request_time > 300) { // 5 minutes
            printf("WeatherApp: Starting periodic refresh (5min timer)\n");
            request_weather_data();
            last_request_time = current_time;
        }
    }
    
    if (is_horizontal) {
        // Horizontal layout using Python PIL reference coordinates
        // These coordinates match the working Python implementation
        
        // Temperature row: (3,3), (13,3), (23,3) - Python coordinates  
        drawText(3, 3, std::to_string(current_weather.min_temp), COLOR_BLUE);   // Low temp
        drawText(13, 3, std::to_string(current_weather.current_temp), COLOR_WHITE); // Current temp
        drawText(23, 3, std::to_string(current_weather.max_temp), COLOR_RED);   // High temp
        
        // Rain row: (3,10), (21,10) - Python coordinates
        drawText(3, 10, "RAIN", COLOR_WHITE);
        drawText(21, 10, std::to_string(current_weather.rain_chance) + "%", COLOR_WHITE);
        
        // Sunrise/sunset row: (3,17), (21,17) - Python coordinates  
        drawText(3, 17, "RISE", COLOR_WHITE);
        drawText(21, 17, current_weather.sunrise, COLOR_WHITE);
        
        // Humidity row: (3,24), (37,24) - Python coordinates
        drawText(3, 24, "HUMIDITY", COLOR_WHITE);
        drawText(37, 24, std::to_string(current_weather.humidity) + "%", COLOR_WHITE);
        
        // Weather icon: (42,1) - 2px up, 2px left from previous position
        ::draw_weather_icon(42, 1, current_weather.icon_code, rotate);
        
        // Show loading indicator if requesting data
        if (api_data_loading) {
            drawText(55, 25, "*", COLOR_WHITE);
        }
        
    } else {
        // Vertical layout (32x64) - Weekly forecast view
        for (int i = 0; i < 5 && i < forecast_data.size(); i++) {
            int y_day = 2 + (i * 12);    // Day name position
            int y_temp = y_day + 6;      // Temperature position (reduced gap from 8 to 6)
            
            // Draw day name in white using vertical rotation (moved 1 pixel right)
            drawText(2, y_day, forecast_data[i].day_name, COLOR_WHITE, RotationMode::VERTICAL_CLOCKWISE);
            
            // Draw low temperature in blue using vertical rotation (moved slightly right)
            drawText(13, y_temp, std::to_string(forecast_data[i].min_temp), COLOR_BLUE, RotationMode::VERTICAL_CLOCKWISE);
            
            // Draw high temperature in red using vertical rotation (moved slightly right)
            drawText(23, y_temp, std::to_string(forecast_data[i].max_temp), COLOR_RED, RotationMode::VERTICAL_CLOCKWISE);
        }
    }
}


void WeatherApp::handle_button_press(bool is_horizontal) {
    // Toggle between current weather and forecast
    sub_state = (sub_state + 1) % 2;
}

