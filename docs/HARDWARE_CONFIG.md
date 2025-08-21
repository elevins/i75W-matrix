# Hardware Configuration

## Display Orientation  
**CRITICAL**: LED matrix is physically mounted UPSIDE DOWN
- All drawing must account for 180° rotation
- This is a hardware mounting requirement, not a software choice
- Horizontal mode: 64w x 32h (after rotation)
- Vertical mode: 32w x 64h (after rotation)

## Hardware Specifications
- **Board**: Raspberry Pi Pico2W (RP2350 with WiFi)
- **LED Driver**: Pimoroni Interstate75W (RM2 module)
- **Display**: 64x32 LED matrix (Hub75 protocol)
- **Color Format**: RGB888 (24-bit color)
- **WiFi Chip**: CYW43 (integrated in Pico2W)
- **Input Controls**:
  - Rotary encoder with quadrature outputs (GPIO 19, 21)
  - Encoder push button (GPIO 20)
  - Tilt switch for orientation detection (GPIO 26)

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

## GPIO Pin Assignments

### Control Pins (Safe from CYW43 conflicts)
```cpp
#define ENCODER_A_PIN 19      // Quadrature A signal
#define ENCODER_B_PIN 21      // Quadrature B signal  
#define ENCODER_SW_PIN 20     // Push button (active low)
#define TILT_SWITCH_PIN 26    // Orientation sensor (active low)
```

### CYW43 WiFi Reserved Pins (DO NOT USE)
- GPIO 23: CYW43_DEFAULT_PIN_WL_REG_ON (power control)
- GPIO 24: CYW43_DEFAULT_PIN_WL_DATA_OUT/IN + HOST_WAKE (SPI + IRQ)
- GPIO 25: CYW43_DEFAULT_PIN_WL_CS (SPI chip select)
- GPIO 29: CYW43_DEFAULT_PIN_WL_CLOCK + VSYS monitoring

## Input Handling - CRITICAL: Use Polling Only
- **Method**: Polling in main loop (10ms intervals)
- **Debouncing**: Software debouncing with microsecond timers
- **Encoder**: Quadrature decoding for direction detection
- **NO INTERRUPTS**: GPIO interrupts conflict with CYW43 WiFi chip

## Build Environment
- **SDK**: Pico SDK with Pimoroni libraries
- **Compiler**: ARM GCC for RP2350
- **Build**: CMake system
- **Critical Libraries**: 
  - `pico_cyw43_arch_lwip_poll` (WiFi - NOT pico_wireless)
  - `hub75` (LED matrix driver)
  - `pico_graphics` (graphics primitives)
  - `pngdec` (PNG image decoding)

## WiFi Integration Requirements
- **CMakeLists.txt**: Must link `pico_cyw43_arch_lwip_poll`
- **Board Config**: `set(PICO_BOARD pico2_w)` 
- **Platform**: `set(PICO_PLATFORM rp2350-arm-s)`
- **Initialization Order**: CYW43 → Hub75 → WiFi Connect

## Power Requirements
- 5V input for LED matrix
- USB power for development
- Consider power consumption with bright colors