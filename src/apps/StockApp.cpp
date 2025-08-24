#include "StockApp.hpp"
#include "../utils/text_renderer.h"

StockApp::StockApp() {
    initialize_stock_data();
}

void StockApp::initialize_stock_data() {
    stock_assets = {
        {"TSLA", "Tesla", "245.30", -3.4f},
        {"NVDA", "Nvidia", "892.15", 5.2f},
        {"AAPL", "Apple", "150.25", 2.1f},
        {"PLTR", "Palantir", "23.67", -1.8f},
        {"SPY", "S&P500", "485.90", 0.9f}
    };
}

void StockApp::draw(bool is_horizontal) {
    if (is_horizontal) {
        // Horizontal: Single asset with large logo and details (stockTicker style)
        int asset_idx = sub_state % stock_assets.size();
        const AssetData& asset = stock_assets[asset_idx];
        draw_single_asset(is_horizontal, asset);
    } else {
        // Vertical: Static list of all stocks
        draw_asset_list(is_horizontal);
    }
}

void StockApp::draw_single_asset(bool is_horizontal, const AssetData& asset) {
    bool rotate = true; // Always rotate 180° for proper orientation
    
    // Top half: Asset symbol and price
    drawText(3, 3, asset.ticker, COLOR_WHITE);          // Asset symbol (white)
    drawText(25, 3, "$" + asset.price.substr(0, 6), COLOR_YELLOW); // Price (yellow)
    
    // Bottom half: Simple 24h change graph visualization
    draw_graph_24h_change(asset.change_24h, rotate);
}

void StockApp::draw_asset_list(bool is_horizontal) {
    // Vertical layout (32x64) - Stock list view matching WeatherApp pattern
    for (int i = 0; i < 5 && i < stock_assets.size(); i++) {
        const AssetData& asset = stock_assets[i];
        int y_symbol = 2 + (i * 12);    // Symbol position
        int y_price = y_symbol + 6;     // Price position (reduced gap)
        
        // Draw stock symbol in white using vertical rotation
        drawText(2, y_symbol, asset.ticker, COLOR_WHITE, RotationMode::VERTICAL_CLOCKWISE);
        
        // Draw current price in white using vertical rotation
        drawText(13, y_price, "$" + asset.price.substr(0, 5), COLOR_WHITE, RotationMode::VERTICAL_CLOCKWISE);
        
        // Draw 24h change with color coding using vertical rotation
        std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
        if (asset.change_24h >= 0) {
            drawText(23, y_symbol, change_str, COLOR_RED, RotationMode::VERTICAL_CLOCKWISE);  // Red for positive
        } else {
            drawText(23, y_symbol, change_str, COLOR_BLUE, RotationMode::VERTICAL_CLOCKWISE); // Blue for negative  
        }
    }
}

void StockApp::draw_graph_24h_change(float change_percent, bool rotate) {
    // Simple bar graph in bottom half (y 16-30)
    int graph_center_y = 23;  // Middle of bottom half
    int graph_start_x = 5;
    int graph_width = 54;     // Most of the width
    
    // Normalize change to graph height (max ±8 pixels from center)
    int max_change = 10;  // Assume max ±10% change for scaling
    int bar_height = (int)((change_percent / max_change) * 8);
    bar_height = std::max(-8, std::min(8, bar_height)); // Clamp to ±8 pixels
    
    // Choose color based on positive/negative
    uint8_t bar_r = change_percent >= 0 ? 0 : 255;
    uint8_t bar_g = change_percent >= 0 ? 255 : 0;
    uint8_t bar_b = 0;
    
    // Draw horizontal baseline
    for (int x = graph_start_x; x < graph_start_x + graph_width; x++) {
        draw_pixel(x, graph_center_y, 100, 100, 100, rotate);
    }
    
    // Draw the change bar
    if (bar_height > 0) {
        // Positive change - bar goes up
        for (int y = graph_center_y - bar_height; y <= graph_center_y; y++) {
            for (int x = graph_start_x + 10; x < graph_start_x + graph_width - 10; x++) {
                draw_pixel(x, y, bar_r, bar_g, bar_b, rotate);
            }
        }
    } else if (bar_height < 0) {
        // Negative change - bar goes down
        for (int y = graph_center_y; y <= graph_center_y - bar_height; y++) {
            for (int x = graph_start_x + 10; x < graph_start_x + graph_width - 10; x++) {
                draw_pixel(x, y, bar_r, bar_g, bar_b, rotate);
            }
        }
    }
    
    // Draw change percentage text
    std::string change_str = (change_percent >= 0 ? "+" : "") + std::to_string(change_percent).substr(0, 4) + "%";
    drawText(25, 10, change_str, bar_r, bar_g, bar_b);
}

void StockApp::handle_button_press(bool is_horizontal) {
    // Only cycle assets when horizontal, disabled when vertical
    if (is_horizontal) {
        sub_state = (sub_state + 1) % stock_assets.size();
    }
}