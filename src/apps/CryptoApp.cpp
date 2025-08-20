#include "CryptoApp.hpp"

CryptoApp::CryptoApp() {
    initialize_crypto_data();
}

void CryptoApp::initialize_crypto_data() {
    crypto_assets = {
        {"BTC", "Bitcoin", "45.2K", 5.7f},
        {"ETH", "Ethereum", "3.2K", -2.1f},
        {"ADA", "Cardano", "0.45", 12.3f},
        {"SOL", "Solana", "105", -4.8f}
    };
}

void CryptoApp::draw(bool is_horizontal) {
    if (is_horizontal) {
        // Horizontal: Single asset with large logo and details (stockTicker style)
        int asset_idx = sub_state % crypto_assets.size();
        const AssetData& asset = crypto_assets[asset_idx];
        draw_single_asset(is_horizontal, asset);
    } else {
        // Vertical: Static list of all cryptos
        draw_asset_list(is_horizontal);
    }
}

void CryptoApp::draw_single_asset(bool is_horizontal, const AssetData& asset) {
    bool rotate = true; // Always rotate 180° for proper orientation
    
    // Large asset logo (left side)
    draw_asset_logo(2, 8, asset.ticker, 255, 165, 0, rotate);
    
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

void CryptoApp::draw_asset_list(bool is_horizontal) {
    bool rotate = true;
    
    draw_string(2, 1, "CRYPTO", 255, 255, 0, rotate);
    
    for (int i = 0; i < 4 && i < crypto_assets.size(); i++) {
        const AssetData& asset = crypto_assets[i];
        int y_pos = 8 + i * 6;
        
        // Small logo + ticker
        draw_string(2, y_pos, asset.ticker, 255, 165, 0, rotate);
        
        // Price
        draw_string(20, y_pos, "$" + asset.price, 255, 255, 255, rotate);
        
        // 24h change with color
        uint8_t change_r = asset.change_24h >= 0 ? 0 : 255;
        uint8_t change_g = asset.change_24h >= 0 ? 255 : 0;
        uint8_t change_b = 0;
        
        std::string change_str = (asset.change_24h >= 0 ? "+" : "") + std::to_string(asset.change_24h).substr(0, 4) + "%";
        draw_string(40, y_pos, change_str, change_r, change_g, change_b, rotate);
    }
}

void CryptoApp::handle_button_press(bool is_horizontal) {
    // Only cycle assets when horizontal, disabled when vertical
    if (is_horizontal) {
        sub_state = (sub_state + 1) % crypto_assets.size();
    }
}