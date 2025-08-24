#include "WeatherApp.hpp"
#include "../utils/text_renderer.h"
#include "../utils/https_client.h"
#include <sstream>
#include <iomanip>

WeatherApp::WeatherApp() : https_client(nullptr), api_data_loaded(false) {
    initialize_mock_data();
    load_weather_icons();
    
    // Initialize HTTPS client
    https_client = new HttpsClient();
    if (https_client && https_client->init("EddyBsHouse", "swiftwater496")) {
        printf("WeatherApp: HTTPS client initialized successfully\n");
        fetch_weather_data();
    } else {
        printf("WeatherApp: Failed to initialize HTTPS client\n");
    }
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

void WeatherApp::draw(bool is_horizontal) {
    // Process HTTPS client network events
    if (https_client) {
        https_client->process();
    }
    
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
            int y_temp = y_day + 6;      // Temperature position (reduced gap from 8 to 6)
            
            // Draw day name in white using vertical rotation (moved 1 pixel right)
            draw_text_white_mode(2, y_day, forecast_data[i].day_name, RotationMode::VERTICAL_CLOCKWISE);
            
            // Draw low temperature in blue using vertical rotation (moved slightly right)
            draw_text_blue_mode(13, y_temp, std::to_string(forecast_data[i].min_temp), RotationMode::VERTICAL_CLOCKWISE);
            
            // Draw high temperature in red using vertical rotation (moved slightly right)
            draw_text_red_mode(23, y_temp, std::to_string(forecast_data[i].max_temp), RotationMode::VERTICAL_CLOCKWISE);
        }
    }
}


WeatherApp::~WeatherApp() {
    if (https_client) {
        delete https_client;
        https_client = nullptr;
    }
}

void WeatherApp::fetch_weather_data() {
    if (!https_client || !https_client->is_connected()) {
        printf("WeatherApp: HTTPS client not ready for request\n");
        return;
    }
    
    // Use OpenWeatherMap API (requires API key)
    // For demo purposes, using a free weather API that supports HTTPS
    std::string url = "https://api.openweathermap.org/data/2.5/weather?q=New York,NY&appid=YOUR_API_KEY&units=imperial";
    
    printf("WeatherApp: Requesting weather data...\n");
    https_client->get(url, [this](const std::string& response) {
        printf("WeatherApp: Received weather response: %s\n", response.c_str());
        parse_weather_response(response);
        api_data_loaded = true;
    });
}

void WeatherApp::parse_weather_response(const std::string& json_response) {
    // Simple JSON parsing - in a real implementation, use a JSON library
    // For now, just update with demo data to show HTTPS is working
    printf("WeatherApp: Parsing weather response (basic implementation)\n");
    
    // Update current weather with API indicator
    current_weather.location = "NYC (API)";
    current_weather.current_temp = 75; // Would parse from JSON
    current_weather.description = "Live Data"; // Would parse from JSON
}

void WeatherApp::handle_button_press(bool is_horizontal) {
    // Button press can trigger a weather data refresh
    if (https_client && https_client->is_connected()) {
        fetch_weather_data();
    }
    // Toggle between current weather and forecast
    sub_state = (sub_state + 1) % 2;
}

