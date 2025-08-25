#pragma once

#include "../core/BaseApp.hpp"

class CryptoApp : public BaseApp {
public:
    CryptoApp();
    ~CryptoApp() = default;
    
    void draw(bool is_horizontal) override;
    void handle_button_press(bool is_horizontal) override;
    
    // Initialize API data immediately on startup
    void initialize_api_data();
    
private:
    std::vector<AssetData> crypto_assets;
    
    void draw_single_asset(bool is_horizontal, const AssetData& asset);
    void draw_asset_list(bool is_horizontal);
    void initialize_crypto_data();
    void draw_graph_24h_change(float change_percent, bool rotate);
};