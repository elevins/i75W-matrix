#!/usr/bin/env python3
"""
Convert tiny.otf font to bitmap C header file for embedded LED matrix use
Based on successful PIL approach from Python weather display implementation
"""

try:
    from PIL import Image, ImageFont, ImageDraw
except ImportError:
    print("PIL not available. Installing...")
    import subprocess
    import sys
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pillow", "--break-system-packages"])
    from PIL import Image, ImageFont, ImageDraw

import os

def generate_char_bitmap(font, char, font_size):
    """Generate bitmap data for a single character at specified font size"""
    # Create image with padding for proper character rendering
    img_size = font_size + 4
    img = Image.new('1', (img_size, img_size), color=0)  # 1-bit monochrome
    draw = ImageDraw.Draw(img)
    
    # Draw character with small offset to avoid clipping
    draw.text((1, 1), char, font=font, fill=1)
    
    # Find actual character bounds to minimize bitmap size
    bbox = img.getbbox()
    if bbox is None:
        # Empty character (space) - return appropriate width
        space_width = max(2, font_size // 2)
        return [], space_width, font_size
    
    # Crop to actual character size
    char_img = img.crop(bbox)
    width, height = char_img.size
    
    # Convert to byte array format (column-major for LED matrix)
    bitmap_data = []
    for y in range(height):
        row_byte = 0
        for x in range(width):
            if char_img.getpixel((x, y)):
                row_byte |= (1 << (7 - (x % 8)))
        bitmap_data.append(row_byte)
    
    return bitmap_data, width, height

def generate_font_header(font_path, output_path, font_size=5):
    """Generate complete C header file with bitmap font data"""
    try:
        font = ImageFont.truetype(font_path, font_size)
        print(f"Loaded font: {font_path} at size {font_size}px")
    except Exception as e:
        print(f"Error loading font: {e}")
        return False
    
    # Generate printable ASCII characters (space to tilde)
    chars = ''.join(chr(i) for i in range(32, 127))
    
    # Header content
    header_content = f"""#pragma once

// Bitmap font data generated from {os.path.basename(font_path)}
// Font size: {font_size}px
// Compatible with LED matrix pixel-by-pixel drawing

#include <cstdint>

struct BitmapChar {{
    const uint8_t* data;
    uint8_t width;
    uint8_t height;
}};

"""
    
    char_data_arrays = []
    char_struct_entries = []
    
    print("Generating character bitmaps...")
    for i, char in enumerate(chars):
        bitmap, width, height = generate_char_bitmap(font, char, font_size)
        
        ascii_val = ord(char)
        
        # Handle empty characters (like space)
        if not bitmap:
            width = max(2, font_size // 2)  # Space width
            height = font_size
            bitmap = [0] * height
        
        # Generate C array for this character
        array_name = f"char_{ascii_val}_data"
        
        if bitmap:
            array_content = f"static const uint8_t {array_name}[] = {{\n"
            for j, byte in enumerate(bitmap):
                if j > 0 and j % 8 == 0:
                    array_content += "\n"
                array_content += f"    0x{byte:02X},"
            array_content = array_content.rstrip(',')  # Remove trailing comma
            array_content += "\n};\n\n"
        else:
            array_content = f"static const uint8_t {array_name}[] = {{0}};\n\n"
        
        char_data_arrays.append(array_content)
        char_struct_entries.append(f"    [{ascii_val}] = {{{array_name}, {width}, {height}}},")
        
        # Show progress for visible characters
        if char.isprintable() and char != ' ':
            print(f"  '{char}' -> {width}x{height} pixels")
    
    # Combine everything
    header_content += "// Character bitmap data arrays\n"
    header_content += "".join(char_data_arrays)
    
    header_content += f"// Character lookup table (indexed by ASCII value)\n"
    header_content += f"static const struct BitmapChar tiny_font_chars[128] = {{\n"
    header_content += "\n".join(char_struct_entries)
    header_content += "\n};\n\n"
    
    # Add utility constants and macros
    header_content += f"// Font properties\n"
    header_content += f"#define TINY_FONT_SIZE {font_size}\n"
    header_content += f"#define TINY_FONT_HEIGHT {font_size}\n"
    header_content += f"#define TINY_FONT_CHARS tiny_font_chars\n\n"
    
    # Add helper function declaration
    header_content += """// Helper function to get character data
inline const BitmapChar* get_char_bitmap(char c) {
    if (c >= 0 && c < 128) {
        return &tiny_font_chars[(int)c];
    }
    return &tiny_font_chars[32]; // Return space for invalid characters
}
"""
    
    # Write to output file
    with open(output_path, 'w') as f:
        f.write(header_content)
    
    print(f"Generated bitmap font header: {output_path}")
    print(f"Font size: {font_size}px")
    print(f"Characters: {len(chars)} (ASCII 32-126)")
    return True

if __name__ == "__main__":
    # Paths relative to project root
    font_path = "src/fonts/tiny.otf"
    output_path = "/tmp/tiny_bitmap.h"
    
    print("=== PIL Font to Bitmap Converter ===")
    print("Converting tiny.otf to C bitmap arrays...")
    
    if not os.path.exists(font_path):
        print(f"Font file not found: {font_path}")
        print("Make sure you're running this from the project root directory")
        exit(1)
    
    # Create output directory if needed
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    success = generate_font_header(font_path, output_path, font_size=5)
    if success:
        print("\n✅ Font conversion completed successfully!")
        print(f"Output: {output_path}")
        print("Ready for use in LED matrix text rendering")
    else:
        print("\n❌ Font conversion failed!")
        exit(1)