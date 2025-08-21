# Claude Memory - i75 LED Matrix Project

## Critical Hardware Requirements
- **ROTATION**: Display is mounted UPSIDE DOWN (180°)  
- **ALWAYS** rotate coordinates in horizontal mode
- **NEVER** change this requirement - hardware is physically upside down
- Screen dimensions: 64x32 pixels
- Hardware: Raspberry Pi Pico2W + Pimoroni Interstate75

## Working Solutions  
- **PNG weather icons**: Fully functional via PNGdec library
  - Uses `draw_weather_icon()` function with pixel-by-pixel drawing
  - Supports transparency and rotation
  - Example: `weather_01d_png_data` embedded C array
- **Custom bitmap fonts**: ✅ WORKING PERFECTLY
  - Uses custom tiny_bitmap.h with 5px bitmap font data
  - Perfect text rendering with `draw_text_white()`, `draw_text_blue()`, `draw_text_red()`
  - Proper 180° rotation handling built into text renderer
  - Matches Python PIL reference layout exactly
- **WiFi + Display Integration**: ✅ WORKING PERFECTLY  
  - CYW43 WiFi chip successfully integrated with Hub75 display
  - Uses `pico_cyw43_arch_lwip_poll` architecture
  - Initialization sequence: CYW43 first, then Hub75, then WiFi connect
- **GPIO Controls**: ✅ WORKING PERFECTLY
  - Polling-based control system (no interrupts)
  - Encoder, button, tilt switch all functional
  - Avoids GPIO interrupt conflicts with CYW43 WiFi chip

## Target Layout (Python PIL Reference - Visual Coordinates)
Based on working Python implementation that produces desired visual layout:
```
Temperature row: (3,3), (13,3), (23,3)     # min, current, max temps
Rain row: (3,10), (21,10)                   # "RAIN" label and percentage  
Sunrise/sunset: (3,17), (21,17)             # "RISE"/"SET" and time
Humidity: (3,24), (37,24)                   # "HUMIDITY" and percentage
Weather icon: (42,1)                        # PNG weather icon position (adjusted)
```
These coordinates represent the final visual appearance after rotation.

## Color System
- Text colors: white (255,255,255)
- Low temperature: blue (100,150,255) 
- High temperature: red (255,100,100)
- RGB888 format for all colors

## Failed Approaches
- **PicoVector/Alright Fonts**: Font loading always failed (red pixel indicator)
  - Tried multiple conversion methods
  - Even with working sample fonts, loading failed
  - User specifically rejected fallback approaches
- **Manual rotation coordinate calculation**: Text positioning broken
  - Overcomplicated coordinate transformations
  - Text appeared off-screen or illegible
- **GPIO Interrupt-based Controls**: Conflicted with CYW43 WiFi chip
  - `gpio_set_irq_enabled_with_callback()` caused display to go blank
  - `gpio_add_raw_irq_handler()` also conflicted with CYW43's interrupt handling
  - CYW43 uses GPIO 24 (HOST_WAKE) with exclusive interrupt priority

## Current Status - ALL SYSTEMS WORKING ✅
- PNG weather icons: ✅ Working perfectly
- Text rendering: ✅ Custom bitmap font system working perfectly
- Layout: ✅ Matches Python reference implementation exactly
- WiFi integration: ✅ CYW43 + Hub75 working together
- GPIO controls: ✅ Polling-based encoder, button, tilt switch
- Complete weather display: ✅ Full horizontal/vertical orientations

## Critical Technical Solutions
1. **WiFi + Display Integration**:
   - Initialize CYW43 first: `cyw43_arch_init()` + `cyw43_arch_enable_sta_mode()`
   - Then initialize Hub75: `hub75.start(dma_complete)`
   - Finally connect WiFi: `cyw43_arch_wifi_connect_timeout_ms()`
   - CMakeLists.txt must link `pico_cyw43_arch_lwip_poll` (not `pico_wireless`)

2. **GPIO Controls Without Interrupts**:
   - Use polling in main loop: `poll_controls()` every 10ms
   - Proper debouncing for each control type
   - Quadrature decoding for rotary encoder
   - GPIO pins: Encoder A=19, B=21, Button=20, Tilt=26

## Development Notes
- Font size: 5px works perfectly in Python PIL version
- Build system: Uses CMake with Pimoroni libraries
- Testing: Build with `make` command
- User prefers direct execution over extensive planning discussions

## Important Decisions
- User explicitly stated "no fallback" approaches
- Custom font must match tiny.otf appearance exactly  
- Project organization: Separated into apps/, core/, fonts/, assets/, utils/
- Documentation: This memory system prevents re-forgetting critical details