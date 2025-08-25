#!/usr/bin/env python3
"""
Font Parser - Convert FONT_EDITOR.txt to C bitmap data
Parses X/. patterns and generates hex values for tiny_bitmap.h
"""

import re
import os

def parse_font_editor():
    """Parse the FONT_EDITOR.txt file and extract all character patterns"""
    font_file = "/home/spencer/Projects/i75/i75-boilerplate/docs/FONT_EDITOR.txt"
    
    if not os.path.exists(font_file):
        print(f"Error: {font_file} not found")
        return {}
    
    characters = {}
    current_ascii = None
    current_rows = []
    
    with open(font_file, 'r') as f:
        for line in f:
            line = line.strip()
            
            # Skip empty lines and comments that don't define characters
            if not line or line.startswith('#') and 'ASCII' not in line:
                continue
            
            # Extract ASCII code from comment lines like "# A (ASCII 65)"
            if line.startswith('#') and 'ASCII' in line:
                match = re.search(r'ASCII (\d+)', line)
                if match:
                    # Save previous character if we have one
                    if current_ascii is not None and len(current_rows) == 5:
                        characters[current_ascii] = current_rows[:]
                    
                    current_ascii = int(match.group(1))
                    current_rows = []
            
            # Extract row patterns like "Row 1: XXX" or "Row 2: X.X"
            elif line.startswith('Row') and ':' in line:
                pattern = line.split(':', 1)[1].strip()
                if len(pattern) == 3:  # Should be exactly 3 pixels wide
                    current_rows.append(pattern)
    
    # Don't forget the last character
    if current_ascii is not None and len(current_rows) == 5:
        characters[current_ascii] = current_rows[:]
    
    return characters

def pattern_to_hex(rows):
    """Convert 5 rows of X/. patterns to hex byte array"""
    hex_values = []
    
    for row in rows:
        byte_val = 0
        for i, pixel in enumerate(row):
            if pixel == 'X':
                # Map 3-pixel positions to bits: leftmost = bit 7, middle = bit 6, right = bit 5
                byte_val |= (1 << (7 - i))
        hex_values.append(f"0x{byte_val:02X}")
    
    return hex_values

def generate_c_code(characters):
    """Generate complete C code for tiny_bitmap.h"""
    
    # Header
    c_code = """#pragma once

// Bitmap font data generated from FONT_EDITOR.txt
// Font size: 3x5 pixels per character
// Compatible with LED matrix pixel-by-pixel drawing

#include <cstdint>

struct BitmapChar {
    const uint8_t* data;
    uint8_t width;
    uint8_t height;
};

// Character bitmap data arrays
"""
    
    # Generate data arrays for all characters
    for ascii_code in sorted(characters.keys()):
        rows = characters[ascii_code]
        hex_values = pattern_to_hex(rows)
        
        if ascii_code == 32:  # Space character needs special handling
            c_code += f"static const uint8_t char_{ascii_code}_data[] = {{0x00, 0x00, 0x00, 0x00, 0x00}};\n\n"
        else:
            c_code += f"static const uint8_t char_{ascii_code}_data[] = {{\n"
            c_code += f"    {', '.join(hex_values)}\n"
            c_code += f"}};\n\n"
    
    # Generate lookup function
    c_code += """// Character lookup function
const BitmapChar* get_char_data(int ascii_code) {
"""
    
    # Generate static character definitions
    for ascii_code in sorted(characters.keys()):
        char_name = f"char_{ascii_code}"
        c_code += f"    static const BitmapChar {char_name} = {{char_{ascii_code}_data, 3, 5}};\n"
    
    c_code += "\n    switch (ascii_code) {\n"
    
    # Generate switch cases
    for ascii_code in sorted(characters.keys()):
        char_name = f"char_{ascii_code}"
        char_desc = chr(ascii_code) if 32 <= ascii_code <= 126 else f"#{ascii_code}"
        c_code += f"        case {ascii_code}: return &{char_name};   // {char_desc}\n"
    
    c_code += """        default: return &char_32;   // fallback to space
    }
}

// Font properties
#define TINY_FONT_SIZE 5
#define TINY_FONT_HEIGHT 5

// Helper function to get character data
inline const BitmapChar* get_char_bitmap(char c) {
    return get_char_data((int)c);
}
"""
    
    return c_code

def main():
    print("Parsing FONT_EDITOR.txt...")
    characters = parse_font_editor()
    
    print(f"Found {len(characters)} characters")
    
    if not characters:
        print("Error: No characters found in FONT_EDITOR.txt")
        return
    
    # Show some examples
    if 65 in characters:  # A
        print("Example - A (65):", characters[65])
        print("Hex values:", pattern_to_hex(characters[65]))
    
    if 48 in characters:  # 0
        print("Example - 0 (48):", characters[48])
        print("Hex values:", pattern_to_hex(characters[48]))
    
    # Generate C code
    print("\nGenerating C code...")
    c_code = generate_c_code(characters)
    
    # Write to tiny_bitmap.h
    output_file = "/home/spencer/Projects/i75/i75-boilerplate/src/utils/tiny_bitmap.h"
    with open(output_file, 'w') as f:
        f.write(c_code)
    
    print(f"Updated {output_file} with {len(characters)} characters")
    print("Font conversion complete!")

if __name__ == "__main__":
    main()