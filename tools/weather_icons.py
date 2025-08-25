#!/usr/bin/env python3
"""
Weather Icon Generator
Creates 17x17 pixel weather icons from existing 21x21 icons (cropped by 2 pixels each side)
Maps Open-Meteo weather codes to icon names
"""

from PIL import Image
import os

# WMO Weather Code to Icon Mapping
WEATHER_CODE_TO_ICON = {
    # Clear
    0: "01d",  # Clear sky
    
    # Cloudy
    1: "02d",  # Mainly clear  
    2: "03d",  # Partly cloudy
    3: "04d",  # Overcast
    
    # Fog
    45: "50d", # Fog
    48: "50d", # Depositing rime fog
    
    # Drizzle
    51: "09d", # Light drizzle
    53: "09d", # Moderate drizzle  
    55: "09d", # Dense drizzle
    
    # Freezing Drizzle
    56: "09d", # Light freezing drizzle
    57: "09d", # Dense freezing drizzle
    
    # Rain
    61: "10d", # Slight rain
    63: "10d", # Moderate rain
    65: "10d", # Heavy rain
    
    # Freezing Rain  
    66: "13d", # Light freezing rain
    67: "13d", # Heavy freezing rain
    
    # Snow
    71: "13d", # Slight snowfall
    73: "13d", # Moderate snowfall
    75: "13d", # Heavy snowfall
    
    # Snow grains
    77: "13d", # Snow grains
    
    # Rain showers
    80: "09d", # Slight rain showers
    81: "09d", # Moderate rain showers
    82: "09d", # Violent rain showers
    
    # Snow showers
    85: "13d", # Slight snow showers
    86: "13d", # Heavy snow showers
    
    # Thunderstorm
    95: "11d", # Thunderstorm
    96: "11d", # Thunderstorm with slight hail
    99: "11d", # Thunderstorm with heavy hail
}

def resize_png_to_17x17(input_path, output_path):
    """Resize a PNG from 21x21 to 17x17 by cropping 2 pixels from each side"""
    try:
        with Image.open(input_path) as img:
            # Check if it's actually 21x21
            if img.size != (21, 21):
                print(f"Warning: {input_path} is not 21x21, it's {img.size}")
            
            # Crop 2 pixels from each side (left, top, right, bottom)
            # Crop box: (left, top, right, bottom)  
            cropped = img.crop((2, 2, 19, 19))  # 21-2=19, so (2,2) to (19,19) gives us 17x17
            
            # Save as PNG
            cropped.save(output_path, 'PNG')
            print(f"Created 17x17 icon: {output_path}")
            
    except Exception as e:
        print(f"Error processing {input_path}: {e}")

def generate_weather_icon_header(png_path, icon_name):
    """Generate C header file for weather icon PNG data"""
    
    try:
        with open(png_path, 'rb') as f:
            png_data = f.read()
        
        header_name = f"weather_{icon_name}_png.h"
        header_path = f"/home/spencer/Projects/i75/i75-boilerplate/src/assets/weather/{header_name}"
        
        # Generate C header content
        c_content = f"#pragma once\n\n"
        c_content += f"unsigned char weather_{icon_name}_png_data[] = {{\n"
        
        # Write binary data as hex values, 12 per line
        for i in range(0, len(png_data), 12):
            chunk = png_data[i:i+12]
            hex_values = [f"0x{b:02x}" for b in chunk]
            c_content += "  " + ", ".join(hex_values) + ",\n"
        
        c_content = c_content.rstrip(',\n') + '\n'  # Remove last comma
        c_content += f"}};\n"
        c_content += f"unsigned int weather_{icon_name}_png_len = {len(png_data)};\n"
        
        with open(header_path, 'w') as f:
            f.write(c_content)
            
        print(f"Generated header: {header_path}")
        
    except Exception as e:
        print(f"Error generating header for {png_path}: {e}")

def main():
    print("Weather Icon Generator - Resizing to 17x17 and generating headers")
    
    assets_dir = "/home/spencer/Projects/i75/i75-boilerplate/src/assets/weather"
    temp_dir = "/tmp/weather_icons"
    os.makedirs(temp_dir, exist_ok=True)
    
    # For now, we'll create a simple 17x17 clear sky icon manually
    # since we only have one existing 21x21 icon
    
    # Create a simple 17x17 clear sky icon
    clear_icon = Image.new('RGBA', (17, 17), (0, 0, 0, 0))  # Transparent background
    
    # Draw a simple yellow sun
    from PIL import ImageDraw
    draw = ImageDraw.Draw(clear_icon)
    
    # Sun center
    sun_color = (244, 154, 50, 255)  # Orange-yellow
    draw.ellipse([5, 5, 11, 11], fill=sun_color)  # 6x6 sun
    
    # Sun rays
    ray_coords = [
        # Top
        (8, 1, 8, 3),
        # Bottom  
        (8, 13, 8, 15),
        # Left
        (1, 8, 3, 8),
        # Right
        (13, 8, 15, 8),
        # Diagonals
        (3, 3, 4, 4),
        (12, 3, 13, 4), 
        (3, 12, 4, 13),
        (12, 12, 13, 13),
    ]
    
    for ray in ray_coords:
        draw.line(ray, fill=sun_color, width=1)
    
    # Save the icon
    clear_icon_path = os.path.join(temp_dir, "01d.png")
    clear_icon.save(clear_icon_path, 'PNG')
    
    # Generate header for clear sky icon
    generate_weather_icon_header(clear_icon_path, "01d")
    
    # Create other basic icons (placeholder for now)
    icons_to_create = [
        ("02d", "partly cloudy"),
        ("03d", "cloudy"), 
        ("04d", "overcast"),
        ("09d", "rain"),
        ("10d", "heavy rain"),
        ("11d", "thunderstorm"),
        ("13d", "snow"),
        ("50d", "fog"),
    ]
    
    for icon_code, description in icons_to_create:
        if icon_code == "01d":
            continue  # Already created
            
        # Create simple placeholder icons
        icon = Image.new('RGBA', (17, 17), (0, 0, 0, 0))
        draw = ImageDraw.Draw(icon)
        
        if "rain" in description:
            # Blue raindrops
            color = (100, 150, 255, 255)
            for x in range(3, 15, 3):
                for y in range(3, 15, 4):
                    draw.ellipse([x, y, x+1, y+2], fill=color)
                    
        elif "cloud" in description:
            # Gray clouds
            color = (150, 150, 150, 255)
            draw.ellipse([2, 6, 14, 10], fill=color)
            draw.ellipse([4, 4, 10, 8], fill=color)
            
        elif "snow" in description:
            # White snowflakes
            color = (255, 255, 255, 255)
            for x in range(4, 14, 4):
                for y in range(4, 14, 4):
                    draw.point((x, y), fill=color)
                    draw.point((x-1, y), fill=color)
                    draw.point((x+1, y), fill=color)
                    draw.point((x, y-1), fill=color) 
                    draw.point((x, y+1), fill=color)
                    
        elif "thunder" in description:
            # Yellow lightning
            color = (255, 255, 0, 255)
            draw.line([(8, 2), (6, 8), (10, 8), (8, 14)], fill=color, width=2)
            
        else:
            # Default gray square
            color = (128, 128, 128, 255)
            draw.rectangle([6, 6, 10, 10], fill=color)
        
        icon_path = os.path.join(temp_dir, f"{icon_code}.png")
        icon.save(icon_path, 'PNG')
        generate_weather_icon_header(icon_path, icon_code)
    
    print(f"\nGenerated {len(icons_to_create)} weather icons")
    print("Weather code mapping:")
    for code, icon in WEATHER_CODE_TO_ICON.items():
        print(f"  {code}: {icon}")

if __name__ == "__main__":
    main()