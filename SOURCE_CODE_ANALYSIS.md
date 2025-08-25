# i75W Matrix Source Code Analysis

## Overview
This analysis covers all source files in the i75W matrix display project, evaluating their purpose, completeness, and recommendations for refactoring.

---

## Core Files

### `src/core/main.cpp` ✅ **ACTIVE**
**Purpose**: Main application entry point and control loop
**Status**: Fully implemented and functional
**Key Features**:
- WiFi initialization and connection management (hardcoded credentials: "EddyBsHouse")
- GPIO control setup (encoder, tilt switch)
- App switching logic (Weather, Stock, Crypto)
- Display update loop at 10Hz
- Polling-based input handling (no interrupts to avoid CYW43 conflicts)

**Analysis**: This is the core orchestrator and appears well-structured. WiFi credentials are hardcoded which may need to be configurable.

### `src/core/common.hpp` ✅ **ACTIVE**  
**Purpose**: Shared types, constants, and function declarations
**Status**: Well-defined interface
**Key Features**:
- Display dimensions (64x32)
- Global graphics objects
- Enums for apps, input status, rotation modes
- Data structures for weather and asset data
- Function declarations for drawing operations

**Analysis**: Good separation of concerns. Clean interface definitions.

### `src/core/common.cpp` ✅ **ACTIVE**
**Purpose**: Shared utility implementations
**Status**: Fully implemented
**Key Features**:
- Pixel drawing with rotation support (180° and vertical)
- Weather icon rendering (PNG fallback to bitmap)
- Asset logo rendering for BTC, ETH, AAPL, TSLA
- WiFi status indicator
- PNGDraw callback implementation

**Analysis**: Complete implementation with good fallback mechanisms for PNG decoding.

### `src/core/BaseApp.hpp` ✅ **ACTIVE**
**Purpose**: Abstract base class for applications  
**Status**: Clean interface design
**Key Features**:
- Pure virtual methods for draw() and handle_button_press()
- Sub-state management
- Consistent interface for all apps

**Analysis**: Good object-oriented design. All apps inherit from this.

---

## Application Files

### `src/apps/WeatherApp.hpp` & `src/apps/WeatherApp.cpp` ⚠️ **PARTIALLY IMPLEMENTED**
**Purpose**: Weather display application
**Status**: Mock data only, no API integration
**Key Features**:
- Horizontal layout: Current weather with temperature, humidity, rain chance
- Vertical layout: 5-day forecast
- Mock data initialization
- Weather icon placeholder logic

**Issues**: 
- Only uses mock data (NYC weather hardcoded)
- PNG icon loading not implemented (`load_weather_icons()` is empty)
- No API calls to weather services

**Recommendation**: **IMPLEMENT** - Core functionality present but needs API integration

### `src/apps/StockApp.hpp` & `src/apps/StockApp.cpp` ⚠️ **PARTIALLY IMPLEMENTED**
**Purpose**: Stock ticker display
**Status**: Mock data only
**Key Features**:
- Horizontal: Single stock with price and 24h change graph
- Vertical: List of 5 stocks with prices and changes
- Mock data for TSLA, NVDA, AAPL, PLTR, SPY
- Simple bar graph visualization

**Issues**:
- Only mock data, no real stock API integration
- Hardcoded stock list

**Recommendation**: **IMPLEMENT** - Good UI framework but needs API integration

### `src/apps/CryptoApp.hpp` & `src/apps/CryptoApp.cpp` ⚠️ **PARTIALLY IMPLEMENTED**
**Purpose**: Cryptocurrency price display
**Status**: Mock data only  
**Key Features**:
- Nearly identical to StockApp but for crypto
- Mock data for BTC, ETH, XNO, DOGE, XMR
- Same graph visualization as stocks

**Issues**:
- Duplicate code with StockApp
- No real crypto API integration

**Recommendation**: **REFACTOR** - Consider consolidating with StockApp into generic AssetApp

---

## Utility Files

### `src/utils/text_renderer.h` & `src/utils/text_renderer.cpp` ✅ **ACTIVE**
**Purpose**: Text rendering with bitmap font
**Status**: Fully implemented
**Key Features**:
- Unified text API with rotation support
- Uses tiny_bitmap.h font data
- Color constants defined
- Text measurement functions

**Analysis**: Complete and well-designed text rendering system.

### `src/utils/network_manager.h` & `src/utils/network_manager.cpp` ❌ **DUPLICATE/UNUSED**
**Purpose**: WiFi and HTTP client management
**Status**: Alternative implementation to main.cpp WiFi handling
**Key Features**:
- NetworkState enum and management
- HTTP GET request handling  
- DNS resolution
- Connection retry with exponential backoff

**Issues**: 
- Not used in main.cpp (which handles WiFi directly)
- Duplicate functionality
- Global reference declared but not used

**Recommendation**: **DELETE** - Redundant with main.cpp WiFi handling

### `src/utils/http_client.h` & `src/utils/http_client.cpp` ❌ **DUPLICATE/UNUSED**
**Purpose**: HTTP/HTTPS client implementation
**Status**: Alternative HTTP implementation
**Key Features**:
- ALTCP-based HTTP/HTTPS support
- DNS resolution  
- TLS support for HTTPS

**Issues**:
- Not used anywhere in the project
- Overlaps with network_manager functionality
- Not integrated with main application

**Recommendation**: **DELETE** - Unused and redundant

### `src/utils/json_parser.h` & `src/utils/json_parser.cpp` ⚠️ **POTENTIAL**
**Purpose**: Simple JSON parsing for API responses
**Status**: Implemented but unused
**Key Features**:
- Basic JSON key-value extraction
- Nested object support
- Array indexing
- Designed for weather API responses

**Analysis**: Well-implemented for its scope. Could be useful for API integration.

**Recommendation**: **KEEP** - Will be needed for API integration in Weather/Stock/Crypto apps

### `src/utils/tiny_bitmap.h` ✅ **ACTIVE**
**Purpose**: 3x5 pixel bitmap font data
**Status**: Complete font implementation  
**Key Features**:
- ASCII characters 32-126
- 3x5 pixel character bitmaps
- Character lookup functions

**Analysis**: Essential for text rendering. Complete implementation.

---

## Asset Files

### `src/assets/weather/*.h` ✅ **ACTIVE**
**Purpose**: Weather icon PNG data as C arrays
**Status**: Complete set of OpenWeatherMap icons
**Files**: 16 icons covering day/night variations of all weather conditions

**Analysis**: Complete icon set, properly embedded as C arrays for easy inclusion.

### `src/assets/weather/*.png` ❌ **REDUNDANT**
**Purpose**: Original PNG files  
**Status**: Source files for the converted headers

**Recommendation**: **DELETE** - Only the .h files are used in the build

---

## Font and Tool Files

### `src/fonts/tiny.otf` ❌ **UNUSED**
**Purpose**: Original font file
**Status**: Source file, not used in build

**Recommendation**: **DELETE** - Font data is embedded in tiny_bitmap.h

### `src/utils/font_parser.py` & `src/utils/weather_icons.py` ❌ **TOOL FILES**  
**Purpose**: Python utilities for asset conversion
**Status**: Development tools

**Recommendation**: **MOVE** - Relocate to tools/ directory or delete if no longer needed

### `src/core/roboto_font.h`, `src/core/tiny_converted_font.h`, `src/core/tiny_font_data.h` ❌ **REDUNDANT**
**Purpose**: Alternative font implementations
**Status**: Not used, superseded by tiny_bitmap.h

**Recommendation**: **DELETE** - tiny_bitmap.h is the active font system

---

## Summary and Recommendations

### Files to DELETE (9 files):
1. `src/utils/network_manager.h` & `.cpp` - Duplicate WiFi handling
2. `src/utils/http_client.h` & `.cpp` - Unused HTTP client  
3. `src/assets/weather/*.png` (16 files) - Redundant PNG sources
4. `src/fonts/tiny.otf` - Unused font file
5. `src/core/roboto_font.h` - Unused font
6. `src/core/tiny_converted_font.h` - Unused font
7. `src/core/tiny_font_data.h` - Unused font

### Files to IMPLEMENT/FIX (3 apps):
1. `WeatherApp` - Add real weather API integration
2. `StockApp` - Add real stock API integration  
3. `CryptoApp` - Add real crypto API integration OR consolidate with StockApp

### Files to RELOCATE (2 files):
1. `src/utils/font_parser.py` → `tools/`
2. `src/utils/weather_icons.py` → `tools/`

### Core Files to KEEP (12 files):
- All `src/core/` files (main.cpp, common.h/cpp, BaseApp.hpp)
- Text renderer (text_renderer.h/cpp)
- Font data (tiny_bitmap.h)
- JSON parser (json_parser.h/cpp) - needed for API work
- Weather icon headers (16 files in src/assets/weather/*.h)

### Total Cleanup:
- **Delete**: 20+ redundant/unused files
- **Keep & enhance**: 12 core files
- **Significant reduction** in codebase complexity while preserving all functional components

The codebase has a solid foundation but needs API integration for the apps and removal of unused/duplicate code for maintainability.