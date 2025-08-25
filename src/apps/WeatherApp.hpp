#pragma once

#include "../core/BaseApp.hpp"
#include <map>
#include <string>

class WeatherApp : public BaseApp {
public:
    WeatherApp();
    ~WeatherApp() = default;
    
    void draw(bool is_horizontal) override;
    void handle_button_press(bool is_horizontal) override;
    
    // Initialize API data immediately on startup
    void initialize_api_data();
    
private:
    WeatherData current_weather;
    std::vector<WeatherData> forecast_data;
    std::map<std::string, uint8_t*> weather_icons;
    bool api_data_loading;
    bool api_data_available;
    bool startup_api_completed;
    
    void load_weather_icons();
    void initialize_mock_data();
    void request_weather_data();
    void parse_weather_response(const std::string& response);
    void on_weather_error(const std::string& error);
};