#pragma once

#include "../core/BaseApp.hpp"

class StockApp : public BaseApp {
public:
    StockApp();
    ~StockApp() = default;
    
    void draw(bool is_horizontal) override;
    void handle_button_press(bool is_horizontal) override;
    
private:
    std::vector<AssetData> stock_assets;
    
    void draw_single_asset(bool is_horizontal, const AssetData& asset);
    void draw_asset_list(bool is_horizontal);
    void initialize_stock_data();
    void draw_graph_24h_change(float change_percent, bool rotate);
};