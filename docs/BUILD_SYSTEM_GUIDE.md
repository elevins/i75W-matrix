# 🏗️ Complete Build System Guide

## 📋 Overview
This guide covers the complete CMake configuration, library dependencies, and build process for the i75W Matrix project with HTTPS support.

---

## 🎯 Project Configuration

### **Platform Settings**
```cmake
cmake_minimum_required(VERSION 3.12)

# Critical platform settings for Pico2W
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_BOARD pico2_w)

# Project name
set(NAME i75-boilerplate)
```

**Why These Settings:**
- `rp2350-arm-s`: Specifies ARM Cortex-M33 architecture for Pico2
- `pico2_w`: Enables WiFi support with CYW43 chip
- Must be set BEFORE `pico_sdk_init()`

### **Source File Organization**
```cmake
add_executable(${NAME}
    src/core/main.cpp
    src/core/common.cpp
    src/apps/WeatherApp.cpp
    src/apps/StockApp.cpp
    src/apps/CryptoApp.cpp
    src/utils/text_renderer.cpp
    src/utils/encoder.cpp
    # HTTPS Service - C wrapper first, then C++ wrapper to ensure proper linking
    src/services/https_c_wrapper.c
    src/services/HTTPSService.cpp
)
```

**Critical Linking Order:**
- C wrapper (`https_c_wrapper.c`) MUST come before C++ wrapper
- This ensures proper symbol resolution for opaque handle pattern

---

## 📚 Library Dependencies

### **Core Libraries (Required)**
```cmake
target_link_libraries(${NAME}
    hub75                        # LED matrix driver
    pico_stdlib                  # Standard Pico functions
    pico_multicore              # Dual-core support (if needed)
    pimoroni_i2c                # I2C support
    pico_graphics               # Graphics primitives
    pico_vector                 # Vector graphics
    pngdec                      # PNG image decoding
    hardware_pwm                # PWM hardware support
)
```

### **Networking Libraries (HTTPS Support)**
```cmake
target_link_libraries(${NAME}
    # WiFi + lwIP stack (poll-based, single-threaded)
    pico_cyw43_arch_lwip_poll   # CYW43 WiFi with lwIP polling
    pico_lwip_mbedtls          # lwIP integration with mbedTLS
    pico_mbedtls               # mbedTLS crypto library
)
```

**Library Linking Order is CRITICAL:**
1. `pico_cyw43_arch_lwip_poll` - WiFi driver + lwIP stack
2. `pico_lwip_mbedtls` - lwIP/mbedTLS integration layer
3. `pico_mbedtls` - Core mbedTLS library

**Wrong Order Results In:**
```
undefined reference to `mbedtls_ssl_set_hostname'
undefined reference to `altcp_tls_create_config_client'
```

### **Include Libraries (Pimoroni)**
```cmake
include(common/pimoroni_i2c)
include(common/pimoroni_bus)
include(libraries/pico_graphics/pico_graphics)
include(libraries/pico_vector/pico_vector)
include(libraries/interstate75/interstate75)
include(libraries/pngdec/pngdec)
```

**Directory Structure Required:**
```
project_root/
├── pimoroni-pico/           # Pimoroni library submodule
│   ├── common/
│   └── libraries/
├── pico-sdk/               # Pico SDK submodule
└── src/                    # Your source code
```

---

## ⚙️ lwIP Configuration (`lwipopts.h`)

### **Essential ALTCP Settings**
```c
// ALTCP (Application Layer TCP) - Required for HTTPS
#define LWIP_ALTCP                  1
#define LWIP_ALTCP_TLS              1
#define LWIP_ALTCP_TLS_MBEDTLS      1

// Memory configuration for embedded systems
#define MEM_SIZE                    4000    // bytes
#define MEM_ALIGNMENT               4       // bytes
#define MEM_LIBC_MALLOC             0       // Use lwIP's allocator
```

### **TCP Configuration**
```c
// TCP settings optimized for HTTPS
#define TCP_MSS                     1460
#define TCP_WND                     (16 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define TCP_WND_UPDATE_THRESHOLD    LWIP_MIN((TCP_WND / 4), (TCP_MSS * 4))
```

### **Network Protocol Stack**
```c
// Enable required protocols
#define LWIP_IPV4                   1
#define LWIP_ARP                    1
#define LWIP_ICMP                   1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1
#define LWIP_UDP                    1
#define LWIP_TCP                    1

// Disable unused features
#define LWIP_IPV6                   0
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0
```

### **Debug Configuration (Development Only)**
```c
// Enable for troubleshooting (disable in production)
#define LWIP_DEBUG                  0
#define ALTCP_MBEDTLS_DEBUG         LWIP_DBG_OFF
#define ALTCP_MBEDTLS_LIB_DEBUG     LWIP_DBG_OFF

// For debugging TLS issues, temporarily enable:
// #define LWIP_DEBUG                  1
// #define ALTCP_MBEDTLS_DEBUG         LWIP_DBG_ON
```

---

## 🔧 Build Configuration

### **Include Directories**
```cmake
# Add lwIP config directory (contains lwipopts.h)
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
```

### **Compiler Standards**
```cmake
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
```

**Why C++17:**
- `std::function` for callbacks
- Better template features
- `auto` type deduction

### **Output Configuration**
```cmake
# Generate UF2 file for flashing
pico_add_extra_outputs(${NAME})

# USB UART output only (not physical UART)
pico_enable_stdio_uart(${NAME} 0)
pico_enable_stdio_usb(${NAME} 1)
```

---

## 🚀 Build Commands

### **Initial Setup**
```bash
# Clone with submodules
git clone --recursive <repository-url>
cd i75W-matrix

# Or if already cloned, get submodules
git submodule update --init --recursive

# Create build directory
mkdir build
cd build
```

### **CMake Configuration**
```bash
# Configure build (first time)
cmake ..

# Or with specific options
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### **Building**
```bash
# Build project
make -j4

# Or with verbose output for debugging
make VERBOSE=1
```

### **Output Files**
After successful build:
```
build/
├── i75-boilerplate.elf      # ELF executable
├── i75-boilerplate.uf2      # UF2 file for flashing
├── i75-boilerplate.bin      # Binary file
├── i75-boilerplate.hex      # Intel hex file
└── i75-boilerplate.map      # Memory map
```

---

## 🐛 Common Build Issues

### **1. Submodule Issues**
**Error:** `Could NOT find PkgConfig`
```bash
# Solution: Initialize submodules
git submodule update --init --recursive
```

### **2. Library Linking Errors**
**Error:** `undefined reference to 'altcp_tls_new'`
**Solution:** Check library linking order:
```cmake
# Correct order:
target_link_libraries(${NAME}
    pico_cyw43_arch_lwip_poll   # First
    pico_lwip_mbedtls          # Second  
    pico_mbedtls               # Third
)
```

### **3. Header Conflicts**
**Error:** `conflicting types for 'local'`
**Solution:** Use C wrapper pattern to isolate headers:
```c
// https_c_wrapper.c - All lwIP headers here only
#include "lwip/altcp_tls.h"
#include "mbedtls/ssl.h"

// No lwIP headers in .hpp files!
```

### **4. Memory Configuration Issues**
**Error:** `region 'RAM' overflowed`
**Solution:** Reduce memory allocations in lwipopts.h:
```c
#define MEM_SIZE                    3000    // Reduce from 4000
#define TCP_WND                     (8 * TCP_MSS)  // Reduce window
```

### **5. Platform Configuration**
**Error:** `Board pico2_w not found`
**Solution:** Verify platform settings at top of CMakeLists.txt:
```cmake
set(PICO_PLATFORM rp2350-arm-s)  # Before pico_sdk_init()
set(PICO_BOARD pico2_w)
```

---

## 🎛️ Advanced Configuration

### **Memory Optimization**
For memory-constrained builds:
```c
// Minimal configuration
#define MEM_SIZE                    2048
#define TCP_MSS                     536
#define TCP_WND                     (4 * TCP_MSS)
#define TCP_SND_BUF                 (2 * TCP_MSS)
#define MEMP_NUM_TCP_SEG            16
#define MEMP_NUM_ARP_QUEUE          5
```

### **Performance Optimization**
For better performance:
```c
// Larger buffers
#define MEM_SIZE                    6000
#define TCP_WND                     (32 * TCP_MSS)
#define TCP_SND_BUF                 (16 * TCP_MSS)
#define MEMP_NUM_TCP_SEG            64
```

### **Debug Configuration**
For development debugging:
```cmake
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Enable lwIP debugging
# In lwipopts.h:
#define LWIP_DEBUG                  1
#define ALTCP_MBEDTLS_DEBUG         LWIP_DBG_ON
#define TCP_DEBUG                   LWIP_DBG_ON
#define DNS_DEBUG                   LWIP_DBG_ON
```

---

## 📊 Build Verification

### **Successful Build Indicators**
```bash
# Should see these outputs:
[100%] Built target i75-boilerplate
# No undefined reference errors
# UF2 file generated
```

### **Flash Size Analysis**
```bash
# Check memory usage
arm-none-eabi-size i75-boilerplate.elf

# Typical output:
#    text    data     bss     dec     hex filename
#  234567    1234   12345  248146   3c952 i75-boilerplate.elf
```

### **Dependency Verification**
```bash
# Check linked libraries
arm-none-eabi-objdump -p i75-boilerplate.elf | grep NEEDED

# Should include mbedTLS and lwIP symbols
```

---

## 🎯 Production Build

### **Release Configuration**
```cmake
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

### **Optimization Settings**
```cmake
# In CMakeLists.txt for production
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
```

### **Size Optimization**
```cmake
# For smallest binary size
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
```

---

## 📦 Package & Deploy

### **Packaging**
```cmake
# Package configuration
set(CPACK_PACKAGE_FILE_NAME ${NAME})
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)

# Build package
make package
```

### **Installation**
```cmake
# Install configuration
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)
```

---

## 🎉 Complete Working Build System

This configuration provides:

**✅ Full HTTPS Support** - lwIP + mbedTLS integration  
**✅ Header Isolation** - C wrapper pattern prevents conflicts  
**✅ Optimal Memory Usage** - Tuned for embedded constraints  
**✅ Reproducible Builds** - Consistent across environments  
**✅ Debug Support** - Easy troubleshooting configuration  
**✅ Production Ready** - Optimized release builds  

The build system has been tested and verified to work with:
- Raspberry Pi Pico2W hardware
- Pimoroni Interstate75W display board  
- 64x32 Hub75 LED matrix
- CYW43 WiFi chip
- lwIP TCP/IP stack
- mbedTLS cryptographic library

**Ready for production deployment! 🚀**