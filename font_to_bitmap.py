#!/usr/bin/env python3

"""
Convert tiny.otf font to bitmap C header file for embedded use
Similar to how we converted PNG to C data
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

def generate_char_bitmap(font, char, size=8):
    """Generate bitmap data for a single character"""
    # Create image slightly larger than font size to catch any overflow
    img_size = size + 4
    img = Image.new('1', (img_size, img_size), color=0)  # 1-bit black/white
    draw = ImageDraw.Draw(img)
    
    # Draw character
    draw.text((1, 1), char, font=font, fill=1)
    
    # Find actual character bounds to minimize bitmap size
    bbox = img.getbbox()
    if bbox is None:
        # Empty character (space)
        return [], 0, 0
    
    # Crop to actual character size
    char_img = img.crop(bbox)
    width, height = char_img.size
    
    # Convert to bitmap data
    bitmap_data = []
    for y in range(height):
        row = 0
        for x in range(width):
            if char_img.getpixel((x, y)):
                row |= (1 << (7 - (x % 8)))
        bitmap_data.append(row)
    
    return bitmap_data, width, height

def generate_font_header(font_path, output_path, font_size=8):
    """Generate complete C header file with font bitmap data"""
    try:
        font = ImageFont.truetype(font_path, font_size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return False
    
    # Characters to generate (printable ASCII)
    chars = ''.join(chr(i) for i in range(32, 127))
    
    header_content = f"""#pragma once

// Bitmap font data generated from {os.path.basename(font_path)}
// Font size: {font_size}px

struct BitmapChar {{
    const uint8_t* data;
    uint8_t width;
    uint8_t height;
}};

"""
    
    char_data_arrays = []
    char_struct_entries = []
    
    for i, char in enumerate(chars):
        bitmap, width, height = generate_char_bitmap(font, char, font_size)
        
        if not bitmap:  # Handle space and empty chars
            width = font_size // 2  # Space width
            height = font_size
            bitmap = [0] * height
        
        # Generate C array for this character
        ascii_val = ord(char)
        array_name = f"char_{ascii_val}_data"
        
        array_content = f"static const uint8_t {array_name}[] = {{\n"
        for byte in bitmap:
            array_content += f"    0x{byte:02X},\n"
        array_content += "};\n\n"
        
        char_data_arrays.append(array_content)
        char_struct_entries.append(f"    [{ascii_val}] = {{{array_name}, {width}, {height}}},")
    
    # Combine everything
    header_content += "// Character bitmap data\n"
    header_content += "".join(char_data_arrays)
    
    header_content += f"// Character lookup table\nstatic const struct BitmapChar tiny_font_chars[128] = {{\n"
    header_content += "\n".join(char_struct_entries)
    header_content += "\n};\n\n"
    
    header_content += f"#define TINY_FONT_SIZE {font_size}\n"
    header_content += "#define TINY_FONT_CHARS tiny_font_chars\n"
    
    # Write to file
    with open(output_path, 'w') as f:
        f.write(header_content)
    
    print(f"Generated bitmap font header: {output_path}")
    return True

if __name__ == "__main__":
    font_path = "src/fonts/tiny.otf"
    output_path = "src/tiny_font_bitmap.h"
    
    if not os.path.exists(font_path):
        print(f"Font file not found: {font_path}")
        exit(1)
    
    success = generate_font_header(font_path, output_path, font_size=6)
    if success:
        print("Font conversion completed!")
    else:
        print("Font conversion failed!")
        exit(1)