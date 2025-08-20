#include "WeatherApp.hpp"
#include "../utils/text_renderer.h"
#include <sstream>
#include <iomanip>

WeatherApp::WeatherApp() {
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
        {"NYC", 75, 68, 82, 55, 10, "02d", "6:46", "7:31", "Partly cloudy", "Mon"},
        {"NYC", 73, 66, 80, 65, 25, "10d", "6:47", "7:30", "Light rain", "Tue"},
        {"NYC", 71, 64, 77, 70, 40, "04d", "6:48", "7:29", "Cloudy", "Wed"},
        {"NYC", 69, 62, 75, 45, 5, "01d", "6:49", "7:28", "Clear", "Thu"},
        {"NYC", 74, 67, 81, 50, 15, "03d", "6:50", "7:27", "Scattered clouds", "Fri"}
    };
}

void WeatherApp::load_weather_icons() {
    // For now, we'll use placeholder logic
    // TODO: Implement PNG loading for weather icons
    // This will require adding PNG decoding capabilities
}

void WeatherApp::draw(bool is_horizontal) {
    
    if (sub_state == 0) {
        draw_current_weather(is_horizontal);
    } else {
        draw_forecast(is_horizontal);
    }
}

void WeatherApp::draw_current_weather(bool is_horizontal) {
    bool rotate = true; // Always rotate 180° for proper orientation
    
    if (is_horizontal) {
        // Horizontal layout using Python PIL reference coordinates
        // These coordinates match the working Python implementation
        
        // Temperature row: (3,3), (13,3), (23,3) - Python coordinates  
        draw_text_blue(3, 3, std::to_string(current_weather.min_temp), rotate);   // Low temp
        draw_text_white(13, 3, std::to_string(current_weather.current_temp), rotate); // Current temp
        draw_text_red(23, 3, std::to_string(current_weather.max_temp), rotate);   // High temp
        
        // Rain row: (3,10), (21,10) - Python coordinates
        draw_text_white(3, 10, "RAIN", rotate);
        draw_text_white(21, 10, std::to_string(current_weather.rain_chance) + "%", rotate);
        
        // Sunrise/sunset row: (3,17), (21,17) - Python coordinates  
        draw_text_white(3, 17, "RISE", rotate);
        draw_text_white(21, 17, current_weather.sunrise, rotate);
        
        // Humidity row: (3,24), (37,24) - Python coordinates
        draw_text_white(3, 24, "HUMIDITY", rotate);
        draw_text_white(37, 24, std::to_string(current_weather.humidity) + "%", rotate);
        
        // Weather icon: (42,1) - 2px up, 2px left from previous position
        ::draw_weather_icon(42, 1, current_weather.icon_code, rotate);
        
    } else {
        // Vertical layout (32x64) - Weekly forecast view
        for (int i = 0; i < 5 && i < forecast_data.size(); i++) {
            int y_day = 2 + (i * 12);    // Day name position
            int y_temp = y_day + 8;      // Temperature position
            
            // Draw day name in white using vertical rotation
            draw_text_white_mode(1, y_day, forecast_data[i].day_name, RotationMode::VERTICAL_CLOCKWISE);
            
            // Draw low temperature in blue using vertical rotation
            draw_text_blue_mode(2, y_temp, std::to_string(forecast_data[i].min_temp), RotationMode::VERTICAL_CLOCKWISE);
            
            // Draw high temperature in red using vertical rotation
            draw_text_red_mode(12, y_temp, std::to_string(forecast_data[i].max_temp), RotationMode::VERTICAL_CLOCKWISE);
        }
    }
}

void WeatherApp::draw_forecast(bool is_horizontal) {
    bool rotate = true;
    
    if (is_horizontal) {
        draw_string(2, 2, "FORECAST", 255, 255, 0, rotate);
        
        for (int i = 0; i < 3 && i < forecast_data.size(); i++) {
            const WeatherData& forecast = forecast_data[i];
            int y_pos = 10 + i * 7;
            
            draw_string(2, y_pos, std::to_string(forecast.min_temp) + "-" + std::to_string(forecast.max_temp), 255, 255, 255, rotate);
            draw_string(25, y_pos, std::to_string(forecast.rain_chance) + "%", 100, 150, 255, rotate);
        }
    } else {
        draw_string(2, 1, "FORECAST", 255, 255, 0, rotate);
        
        for (int i = 0; i < 3 && i < forecast_data.size(); i++) {
            const WeatherData& forecast = forecast_data[i];
            int y_pos = 8 + i * 8;
            
            draw_string(2, y_pos, "DAY" + std::to_string(i+1), 200, 200, 200, rotate);
            draw_string(2, y_pos + 3, std::to_string(forecast.max_temp) + "F", 255, 255, 255, rotate);
            draw_string(25, y_pos + 3, std::to_string(forecast.rain_chance) + "%", 100, 150, 255, rotate);
        }
    }
}

void WeatherApp::handle_button_press(bool is_horizontal) {
    // Toggle between current weather and forecast
    sub_state = (sub_state + 1) % 2;
}

