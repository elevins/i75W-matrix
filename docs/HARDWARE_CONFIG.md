# Hardware Configuration

## Display Orientation  
**CRITICAL**: LED matrix is physically mounted UPSIDE DOWN
- All drawing must account for 180° rotation
- This is a hardware mounting requirement, not a software choice
- Horizontal mode: 64w x 32h (after rotation)
- Vertical mode: 32w x 64h (after rotation)

## Hardware Specifications
- **Board**: Raspberry Pi Pico2W
- **LED Driver**: Pimoroni Interstate75  
- **Display**: 64x32 LED matrix
- **Color Format**: RGB888 (24-bit color)
- **Input**: Rotary encoder with button

## Rotation Mathematics
For upside-down display (180° rotation):
```
physical_x = 63 - visual_x
physical_y = 31 - visual_y
```

## Color Definitions
```cpp
// Standard colors used in weather app
RGB white = {255, 255, 255};      // Primary text
RGB blue = {100, 150, 255};       // Low temperature
RGB red = {255, 100, 100};        // High temperature  
RGB yellow = {255, 255, 0};       // Accent/highlight
RGB green = {0, 255, 0};          // Status indicator
```

## Input Handling
- **Encoder rotation**: CW/CCW for navigation
- **Button press**: Mode switching
- **Debouncing**: Required for clean input detection

## Build Environment
- **SDK**: Pico SDK with Pimoroni libraries
- **Compiler**: ARM GCC for RP2350
- **Build**: CMake system
- **Libraries**: pico_graphics, hub75, pngdec, pico_vector

## Power Requirements
- 5V input for LED matrix
- USB power for development
- Consider power consumption with bright colors