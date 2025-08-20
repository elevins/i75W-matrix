#include "json_parser.h"
#include <algorithm>
#include <sstream>

JsonParser::JsonParser(const std::string& json_str) : json_data(json_str) {
    // Remove whitespace for easier parsing
    json_data.erase(std::remove_if(json_data.begin(), json_data.end(), ::isspace), json_data.end());
}

std::string JsonParser::findValue(const std::string& key) const {
    std::string search_key = "\"" + key + "\":";
    size_t key_pos = json_data.find(search_key);
    
    if (key_pos == std::string::npos) {
        return "";
    }
    
    size_t value_start = key_pos + search_key.length();
    size_t value_end = value_start;
    
    // Find end of value
    if (json_data[value_start] == '"') {
        // String value
        value_end = json_data.find('"', value_start + 1);
        if (value_end != std::string::npos) {
            value_end++;
        }
    } else if (json_data[value_start] == '[') {
        // Array value - find matching closing bracket
        int bracket_count = 1;
        value_end = value_start + 1;
        while (value_end < json_data.length() && bracket_count > 0) {
            if (json_data[value_end] == '[') bracket_count++;
            else if (json_data[value_end] == ']') bracket_count--;
            value_end++;
        }
    } else if (json_data[value_start] == '{') {
        // Object value - find matching closing brace
        int brace_count = 1;
        value_end = value_start + 1;
        while (value_end < json_data.length() && brace_count > 0) {
            if (json_data[value_end] == '{') brace_count++;
            else if (json_data[value_end] == '}') brace_count--;
            value_end++;
        }
    } else {
        // Number or boolean - find next comma, bracket, or brace
        while (value_end < json_data.length() && 
               json_data[value_end] != ',' && 
               json_data[value_end] != '}' && 
               json_data[value_end] != ']') {
            value_end++;
        }
    }
    
    if (value_end == std::string::npos || value_end <= value_start) {
        return "";
    }
    
    return json_data.substr(value_start, value_end - value_start);
}

std::string JsonParser::getNestedValue(const std::string& path) const {
    size_t dot_pos = path.find('.');
    if (dot_pos == std::string::npos) {
        return findValue(path);
    }
    
    std::string parent_key = path.substr(0, dot_pos);
    std::string child_path = path.substr(dot_pos + 1);
    
    std::string parent_obj = findValue(parent_key);
    if (parent_obj.empty() || parent_obj[0] != '{') {
        return "";
    }
    
    // Create parser for nested object
    JsonParser nested_parser(parent_obj);
    return nested_parser.getNestedValue(child_path);
}

std::string JsonParser::extractString(const std::string& value) const {
    if (value.length() >= 2 && value[0] == '"' && value[value.length()-1] == '"') {
        return value.substr(1, value.length() - 2);
    }
    return value;
}

double JsonParser::extractNumber(const std::string& value) const {
    if (value.empty()) {
        return 0.0;
    }
    
    // Simple manual parsing instead of std::stod
    double result = 0.0;
    double sign = 1.0;
    size_t pos = 0;
    
    // Handle negative numbers
    if (value[0] == '-') {
        sign = -1.0;
        pos = 1;
    }
    
    // Parse integer part
    while (pos < value.length() && value[pos] >= '0' && value[pos] <= '9') {
        result = result * 10.0 + (value[pos] - '0');
        pos++;
    }
    
    // Parse decimal part
    if (pos < value.length() && value[pos] == '.') {
        pos++;
        double fraction = 0.1;
        while (pos < value.length() && value[pos] >= '0' && value[pos] <= '9') {
            result += (value[pos] - '0') * fraction;
            fraction *= 0.1;
            pos++;
        }
    }
    
    return result * sign;
}

std::string JsonParser::getString(const std::string& path) const {
    std::string value = getNestedValue(path);
    return extractString(value);
}

double JsonParser::getNumber(const std::string& path) const {
    std::string value = getNestedValue(path);
    return extractNumber(value);
}

int JsonParser::getInt(const std::string& path) const {
    return (int)getNumber(path);
}

double JsonParser::getArrayNumber(const std::string& path, int index) const {
    std::string array_value = getNestedValue(path);
    
    if (array_value.empty() || array_value[0] != '[') {
        return 0.0;
    }
    
    // Simple array parsing - find nth comma-separated value
    int current_index = 0;
    size_t pos = 1; // Skip opening bracket
    
    while (pos < array_value.length() && current_index < index) {
        if (array_value[pos] == ',') {
            current_index++;
        }
        pos++;
    }
    
    if (current_index != index) {
        return 0.0;
    }
    
    // Find end of current value
    size_t value_start = pos;
    while (pos < array_value.length() && 
           array_value[pos] != ',' && 
           array_value[pos] != ']') {
        pos++;
    }
    
    std::string value = array_value.substr(value_start, pos - value_start);
    return extractNumber(value);
}

bool JsonParser::hasKey(const std::string& path) const {
    return !getNestedValue(path).empty();
}