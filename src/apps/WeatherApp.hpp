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
    
private:
    WeatherData current_weather;
    std::vector<WeatherData> forecast_data;
    std::map<std::string, uint8_t*> weather_icons;
    
    void load_weather_icons();
    void initialize_mock_data();
};