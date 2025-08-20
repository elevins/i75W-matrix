#include "StockApp.hpp"

StockApp::StockApp() {
    initialize_stock_data();
}

void StockApp::initialize_stock_data() {
    stock_assets = {
        {"AAPL", "Apple", "150.25", 2.1f},
        {"GOOGL", "Google", "2800.50", -1.2f},
        {"MSFT", "Microsoft", "375.80", 0.8f},
        {"TSLA", "Tesla", "245.30", -3.4f}
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
    
    // Large asset logo (left side)
    draw_asset_logo(2, 8, asset.ticker, 255, 255, 255, rotate);
    
    // Asset name and ticker (top right)
    draw_string(12, 2, asset.name, 255, 255, 0, rotate);
    draw_string(12, 10, asset.ticker, 200, 200, 200, rotate);
    
    // Current price (large, middle right)
    draw_string(12, 18, "$" + asset.price, 255, 255, 255, rotate);
    
    // 24h change with color coding (bottom right)
    uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
    uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
    uint8_t change_b = 0;
    
    std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
    draw_string(12, 26, change_str, change_r, change_g, change_b, rotate);
}

void StockApp::draw_asset_list(bool is_horizontal) {
    bool rotate = true;
    
    draw_string(2, 1, "STOCKS", 255, 255, 0, rotate);
    
    for (int i = 0; i < 4 && i < stock_assets.size(); i++) {
        const AssetData& asset = stock_assets[i];
        int y_pos = 8 + i * 6;
        
        // Small logo + ticker
        draw_string(2, y_pos, asset.ticker, 200, 200, 200, rotate);
        
        // Price
        draw_string(24, y_pos, "$" + asset.price.substr(0, 6), 255, 255, 255, rotate);
        
        // 24h change with color
        uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
        uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
        uint8_t change_b = 0;
        
        std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
        draw_string(45, y_pos, change_str, change_r, change_g, change_b, rotate);
    }
}

void StockApp::handle_button_press(bool is_horizontal) {
    // Only cycle assets when horizontal, disabled when vertical
    if (is_horizontal) {
        sub_state = (sub_state + 1) % stock_assets.size();
    }
}