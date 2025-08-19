#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/interstate75/interstate75.hpp"

using namespace pimoroni;

// GPIO pin definitions
#define ENCODER_A_PIN 19
#define ENCODER_B_PIN 21
#define ENCODER_SW_PIN 20
#define TILT_SWITCH_PIN 26

// Display size for 64x32 HUB75 panel
Hub75 hub75(64, 32, nullptr, PANEL_GENERIC, false);

// PicoGraphics surface for 64x32 display
PicoGraphics_PenRGB888 graphics(64, 32, nullptr);

// Display dimensions
const int width = 64;
const int height = 32;

// Global variables for input handling
volatile int encoder_pos = 0;
volatile bool encoder_pressed = false;
volatile bool tilt_active = false;

// Encoder state tracking
static int last_a = 0;
static int last_b = 0;

// GPIO interrupt handler for rotary encoder
void encoder_interrupt(uint gpio, uint32_t events) {
    int a = gpio_get(ENCODER_A_PIN);
    int b = gpio_get(ENCODER_B_PIN);
    
    if (gpio == ENCODER_A_PIN || gpio == ENCODER_B_PIN) {
        if (a != last_a) {
            if (a != b) {
                encoder_pos++;
            } else {
                encoder_pos--;
            }
        }
        last_a = a;
        last_b = b;
    }
    
    if (gpio == ENCODER_SW_PIN && events & GPIO_IRQ_EDGE_FALL) {
        encoder_pressed = !encoder_pressed;
    }
    
    if (gpio == TILT_SWITCH_PIN) {
        tilt_active = !gpio_get(TILT_SWITCH_PIN);
    }
}

// Function to rotate coordinates 180 degrees for upside-down display
Point rotate_180(int x, int y) {
    return Point(width - 1 - x, height - 1 - y);
}

// Safe pixel drawing with 180-degree rotation
void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        graphics.set_pen(r, g, b);
        Point rotated = rotate_180(x, y);
        graphics.pixel(rotated);
    }
}

// Interrupt callback required function 
void __isr dma_complete() {
  hub75.dma_complete();
}


int main() {
    stdio_init_all();

    // Initialize GPIO pins
    gpio_init(ENCODER_A_PIN);
    gpio_init(ENCODER_B_PIN);
    gpio_init(ENCODER_SW_PIN);
    gpio_init(TILT_SWITCH_PIN);
    
    gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
    gpio_set_dir(ENCODER_B_PIN, GPIO_IN);
    gpio_set_dir(ENCODER_SW_PIN, GPIO_IN);
    gpio_set_dir(TILT_SWITCH_PIN, GPIO_IN);
    
    gpio_pull_up(ENCODER_A_PIN);
    gpio_pull_up(ENCODER_B_PIN);
    gpio_pull_up(ENCODER_SW_PIN);
    gpio_pull_up(TILT_SWITCH_PIN);

    // Set up GPIO interrupts
    gpio_set_irq_enabled_with_callback(ENCODER_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &encoder_interrupt);
    gpio_set_irq_enabled(ENCODER_B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ENCODER_SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(TILT_SWITCH_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Initialize encoder state
    last_a = gpio_get(ENCODER_A_PIN);
    last_b = gpio_get(ENCODER_B_PIN);

    // Start hub75 driver
    hub75.start(dma_complete);

    // Main variables
    int cursor_x = width / 2;
    int cursor_y = height / 2;
    static int last_encoder_pos = 0;

    while(true) {
        // Clear screen
        graphics.set_pen(0, 0, 0);
        graphics.clear();

        // Handle encoder position changes
        if (encoder_pos != last_encoder_pos) {
            int delta = encoder_pos - last_encoder_pos;
            cursor_x += delta;
            
            // Wrap cursor around screen edges
            if (cursor_x < 0) cursor_x = width - 1;
            if (cursor_x >= width) cursor_x = 0;
            
            last_encoder_pos = encoder_pos;
        }

        // Draw cursor (changes color based on tilt switch)
        if (tilt_active) {
            draw_pixel(cursor_x, cursor_y, 255, 0, 0); // Red when tilted
        } else {
            draw_pixel(cursor_x, cursor_y, 0, 255, 0); // Green when not tilted
        }

        // Draw a border around the screen
        for (int x = 0; x < width; x++) {
            draw_pixel(x, 0, 128, 128, 128);
            draw_pixel(x, height - 1, 128, 128, 128);
        }
        for (int y = 0; y < height; y++) {
            draw_pixel(0, y, 128, 128, 128);
            draw_pixel(width - 1, y, 128, 128, 128);
        }

        // Draw some text info (basic pattern)
        if (encoder_pressed) {
            // Draw a cross pattern when button is pressed
            for (int i = 0; i < width; i++) {
                draw_pixel(i, height / 2, 0, 0, 255);
            }
            for (int i = 0; i < height; i++) {
                draw_pixel(width / 2, i, 0, 0, 255);
            }
        }

        // Update display
        hub75.update(&graphics);
        sleep_ms(50);
    }

    return 0;
}