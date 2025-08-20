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
- **Built-in bitmap fonts**: Work but positioning issues with rotation
  - Uses font6 (smallest available)
  - Current implementation has text positioning problems

## Target Layout (Python PIL Reference - Visual Coordinates)
Based on working Python implementation that produces desired visual layout:
```
Temperature row: (3,3), (13,3), (23,3)     # min, current, max temps
Rain row: (3,10), (21,10)                   # "RAIN" label and percentage  
Sunrise/sunset: (3,17), (21,17)             # "RISE"/"SET" and time
Humidity: (3,24), (37,24)                   # "HUMIDITY" and percentage
Weather icon: (40,1)                        # PNG weather icon position
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

## Current Status
- PNG weather icons: ✅ Working perfectly
- Text rendering: ❌ Needs custom bitmap font solution
- Layout: ✅ Have proven Python reference implementation

## Next Steps - Bitmap Font Implementation
1. Use PIL to convert tiny.otf at 5px to bitmap character arrays
2. Implement custom text renderer using pixel-by-pixel approach (like PNG icons)
3. Use Python layout coordinates as visual targets
4. Handle rotation internally in text renderer

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