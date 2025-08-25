#pragma once

#include "pico/stdlib.h"
#include <cstdint>

enum class EncoderResult {
    NONE = 0,           // No change
    CLOCKWISE = 1,      // One step clockwise
    COUNTER_CLOCKWISE = 2 // One step counter-clockwise
};

class KY040Encoder {
public:
    void init(uint clk_pin, uint dt_pin);
    EncoderResult process();
    int32_t get_position() const { return position_; }
    void reset_position() { position_ = 0; }

private:
    uint clk_pin_;      // CLK pin (Encoder A)
    uint dt_pin_;       // DT pin (Encoder B)
    uint8_t state_;     // Current state in state machine
    int32_t position_;  // Current encoder position
    uint32_t last_change_time_; // Last valid state change time for debouncing

    static constexpr uint32_t DEBOUNCE_US = 5000;   // 5ms debounce for KY-040
    static constexpr uint32_t READ_INTERVAL = 1000; // 1ms polling interval

    // State machine definitions (Buxtronix half-step mode)
    #define R_START     0x0
    #define R_CW_FINAL  0x1
    #define R_CW_BEGIN  0x2
    #define R_CW_NEXT   0x3
    #define R_CCW_BEGIN 0x4
    #define R_CCW_FINAL 0x5
    #define R_CCW_NEXT  0x6

    static const uint8_t ttable[7][4];
    uint8_t read_pins();
};

extern KY040Encoder ky040_encoder;