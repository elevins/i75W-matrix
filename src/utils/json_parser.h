#pragma once

#include <string>
#include <map>

// Simple JSON parser for weather data
// Supports basic key-value extraction for our specific use case
class JsonParser {
public:
    JsonParser(const std::string& json_str);
    
    // Get string value by path (e.g., "current.temperature_2m")
    std::string getString(const std::string& path) const;
    
    // Get numeric value by path
    double getNumber(const std::string& path) const;
    
    // Get integer value by path
    int getInt(const std::string& path) const;
    
    // Get array element (e.g., "daily.temperature_2m_max[0]")
    double getArrayNumber(const std::string& path, int index) const;
    
    // Check if key exists
    bool hasKey(const std::string& path) const;
    
private:
    std::string json_data;
    
    // Helper functions
    std::string findValue(const std::string& key) const;
    std::string extractString(const std::string& value) const;
    double extractNumber(const std::string& value) const;
    std::string getNestedValue(const std::string& path) const;
};