#include "encoder.hpp"
#include <cstdio>


KY040Encoder ky040_encoder; // Global instance

const uint8_t KY040Encoder::ttable[7][4] = {
    {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
    {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | 0x10},
    {R_CW_BEGIN, R_CW_BEGIN,  R_START,     R_START},
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
    {R_CCW_BEGIN, R_START,     R_CCW_BEGIN, R_START},
    {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | 0x20},
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

void KY040Encoder::init(uint clk_pin, uint dt_pin) {
    clk_pin_ = clk_pin;
    dt_pin_ = dt_pin;
    position_ = 0;
    state_ = R_START;
    last_change_time_ = 0;

    gpio_init(clk_pin_);
    gpio_init(dt_pin_);
    gpio_set_dir(clk_pin_, GPIO_IN);
    gpio_set_dir(dt_pin_, GPIO_IN);
    // Enable pull-ups only if KY-040 lacks them (uncomment if needed)
    // gpio_pull_up(clk_pin_);
    // gpio_pull_up(dt_pin_);

    state_ = read_pins();
    printf("Encoder initialized: CLK=%d (GPIO%d), DT=%d (GPIO%d), initial state=%d\n", 
           gpio_get(clk_pin_), clk_pin_, gpio_get(dt_pin_), dt_pin_, state_);
}

uint8_t KY040Encoder::read_pins() {
    return (gpio_get(clk_pin_) | (gpio_get(dt_pin_) << 1));
}

EncoderResult KY040Encoder::process() {
    uint32_t current_time = time_us_32();

    uint8_t pin_state = read_pins();
    printf("Encoder pins: CLK=%d, DT=%d (pin_state=%d)\n", 
           gpio_get(clk_pin_), gpio_get(dt_pin_), pin_state);

    if (current_time - last_change_time_ < DEBOUNCE_US) {
        return EncoderResult::NONE;
    }

    uint8_t old_state = state_;
    state_ = ttable[state_ & 0x0f][pin_state];
    printf("Encoder state: %d -> %d\n", old_state, state_);

    if (state_ & 0x10) {
        state_ &= 0x0f;
        position_++;
        last_change_time_ = current_time;
        printf("CW: pos=%d\n", position_);
        return EncoderResult::CLOCKWISE;
    }
    if (state_ & 0x20) {
        state_ &= 0x0f;
        position_--;
        last_change_time_ = current_time;
        printf("CCW: pos=%d\n", position_);
        return EncoderResult::COUNTER_CLOCKWISE;
    }

    return EncoderResult::NONE;
}