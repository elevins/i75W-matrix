#pragma once

#include "../core/BaseApp.hpp"
#include <map>
#include <string>

// Forward declaration to avoid include conflicts
class HttpsClient;

class WeatherApp : public BaseApp {
public:
    WeatherApp();
    ~WeatherApp();
    
    void draw(bool is_horizontal) override;
    void handle_button_press(bool is_horizontal) override;
    
private:
    WeatherData current_weather;
    std::vector<WeatherData> forecast_data;
    std::map<std::string, uint8_t*> weather_icons;
    HttpsClient* https_client;
    bool api_data_loaded;
    
    void load_weather_icons();
    void initialize_mock_data();
    void fetch_weather_data();
    void parse_weather_response(const std::string& json_response);
};