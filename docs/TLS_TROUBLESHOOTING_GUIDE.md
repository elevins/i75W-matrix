# 🔧 TLS/HTTPS Troubleshooting Guide for Embedded Systems

## 🚨 Critical Issue: ALTCP Error -15 (TLS Handshake Failure)

### **Problem**: 
```
ALTCP error -15: TLS handshake failed (connection closed during handshake)
```

### **Root Cause Analysis**:
Modern HTTPS servers require **SNI (Server Name Indication)** to select the correct SSL certificate. Without SNI, the server doesn't know which certificate to present for the domain, causing the TLS handshake to fail.

### **The Solution That Works** ⭐:
```c
// CRITICAL: Get TLS context and set SNI hostname
void* tls_context = altcp_tls_context(request->pcb);
if (tls_context) {
    mbedtls_ssl_context* ssl = (mbedtls_ssl_context*)tls_context;
    int sni_ret = mbedtls_ssl_set_hostname(ssl, "api.open-meteo.com");
    if (sni_ret == 0) {
        printf("SNI hostname set successfully\n");
    }
}
```

---

## 🔍 Complete lwIP Error Code Reference

### **Memory Errors**
| Code | Error | Meaning | Solution |
|------|-------|---------|----------|
| -1 | `ERR_MEM` | Out of memory | Increase `MEM_SIZE` in lwipopts.h |
| -2 | `ERR_BUF` | Buffer error | Check buffer sizes, increase TCP buffers |

### **Network Errors**  
| Code | Error | Meaning | Solution |
|------|-------|---------|----------|
| -3 | `ERR_TIMEOUT` | Operation timed out | Check network connectivity, increase timeouts |
| -4 | `ERR_RTE` | Routing problem | Verify WiFi connection and gateway |
| -11 | `ERR_CONN` | Not connected | Ensure connection established before sending data |
| -12 | `ERR_IF` | Low-level network error | Check WiFi driver and hardware |

### **Connection State Errors**
| Code | Error | Meaning | Solution |
|------|-------|---------|----------|
| -5 | `ERR_INPROGRESS` | Operation in progress | Wait for completion, don't retry immediately |
| -8 | `ERR_USE` | Address in use | Wait before retrying connection |
| -9 | `ERR_ALREADY` | Already connected | Check connection state before connecting |
| -10 | `ERR_ISCONN` | Already connected | Same as above |

### **TLS-Specific Errors**
| Code | Error | Meaning | Solution |
|------|-------|---------|----------|
| -13 | `ERR_ABRT` | Connection aborted | Often certificate/TLS config issue |
| -14 | `ERR_RST` | Connection reset | Server rejected connection |
| **-15** | **`ERR_CLSD`** | **TLS handshake failed** | **Add SNI support** ⭐ |
| -16 | `ERR_ARG` | Invalid argument | Check function parameters |

---

## 🔐 TLS Configuration for Embedded Systems

### **1. Disable Certificate Verification** (Essential for Embedded)
```c
// Embedded systems don't have root CA certificate bundles
mbedtls_ssl_conf_authmode(ssl->conf, MBEDTLS_SSL_VERIFY_NONE);
printf("Certificate verification disabled for embedded compatibility\n");
```

**Why This Is Needed:**
- Embedded systems lack the ~150KB+ root certificate bundle
- Certificate validation requires significant RAM and processing
- For many IoT applications, encryption is more important than authentication

### **2. Set Minimum TLS Version**
```c
// Ensure compatibility with modern servers (TLS 1.2+)
mbedtls_ssl_conf_min_version(ssl->conf, 
    MBEDTLS_SSL_MAJOR_VERSION_3,    // TLS 1.x 
    MBEDTLS_SSL_MINOR_VERSION_3);   // TLS 1.2
printf("Minimum TLS version set to 1.2\n");
```

### **3. Complete Working TLS Setup**
```c
// 1. Create TLS config
struct altcp_tls_config* tls_config = altcp_tls_create_config_client(NULL, 0);

// 2. Create ALTCP PCB with TLS
struct altcp_pcb* pcb = altcp_tls_new(tls_config, IP_GET_TYPE(ipaddr));

// 3. Get TLS context and configure
void* tls_context = altcp_tls_context(pcb);
if (tls_context) {
    mbedtls_ssl_context* ssl = (mbedtls_ssl_context*)tls_context;
    
    // CRITICAL: Set SNI hostname
    mbedtls_ssl_set_hostname(ssl, hostname);
    
    // Configure for embedded system  
    mbedtls_ssl_conf_authmode(ssl->conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_min_version(ssl->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
}

// 4. Connect
altcp_connect(pcb, ipaddr, 443, connected_callback);
```

---

## 📊 Diagnostic Console Output

### **❌ FAILED Connection (Before SNI Fix):**
```
DNS resolved for api.open-meteo.com
Creating TLS client config for hostname: api.open-meteo.com
Initiating TLS connection to api.open-meteo.com:443
TLS connection initiated successfully, waiting for handshake...
ALTCP error -15: TLS handshake failed (connection closed during handshake)
=== WEATHER API ERROR ===
Error: TLS handshake failed (connection closed during handshake)
=== END ERROR ===
```

### **✅ SUCCESSFUL Connection (After SNI Fix):**
```
DNS resolved for api.open-meteo.com
Creating TLS client config for hostname: api.open-meteo.com
SNI hostname set to: api.open-meteo.com                    ← KEY DIFFERENCE!
Certificate verification disabled for embedded compatibility
Minimum TLS version set to 1.2
Initiating TLS connection to api.open-meteo.com:443 with SNI support
TLS connection initiated successfully, waiting for handshake...
TLS handshake completed successfully! Sending HTTP request  ← SUCCESS!
HTTP request sent
Data sent: 298 bytes
Connection closed, processing response
```

---

## 🛠️ Step-by-Step Debugging Process

### **Step 1: Verify Basic Connectivity**
```c
// Check if WiFi is connected
if (!cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)) {
    printf("ERROR: WiFi not connected\n");
    return;
}

// Check if we can resolve DNS
printf("Resolving DNS for %s...\n", hostname);
// DNS resolution will show in logs
```

### **Step 2: Enable TLS Debug Logging**
```c
// In lwipopts.h - temporarily enable for debugging
#define LWIP_DEBUG                  1
#define ALTCP_MBEDTLS_DEBUG         LWIP_DBG_ON
#define ALTCP_MBEDTLS_LIB_DEBUG     LWIP_DBG_ON
```

### **Step 3: Check Memory Configuration**
```c
// Verify sufficient memory allocated
#define MEM_SIZE                    4000    // bytes - increase if needed
#define TCP_WND                     (16 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
```

### **Step 4: Test with Known Working Server**
Start with a simple HTTPS server:
```c
// Test with a simple endpoint first
const char* test_hostname = "httpbin.org";
const char* test_path = "/get";
```

### **Step 5: Verify SNI Implementation**
```c
void* tls_context = altcp_tls_context(pcb);
if (!tls_context) {
    printf("ERROR: Could not get TLS context\n");
    return;
}

mbedtls_ssl_context* ssl = (mbedtls_ssl_context*)tls_context;
if (!ssl) {
    printf("ERROR: Invalid SSL context\n");
    return;
}

int sni_ret = mbedtls_ssl_set_hostname(ssl, hostname);
if (sni_ret != 0) {
    printf("ERROR: SNI setup failed: %d\n", sni_ret);
} else {
    printf("SUCCESS: SNI hostname set to: %s\n", hostname);
}
```

---

## ⚡ Performance Optimization

### **Memory Usage Optimization**
```c
// Minimal memory configuration for small embedded systems
#define MEM_SIZE                    2048    // Reduce if very memory constrained
#define TCP_MSS                     536     // Smaller MSS for limited bandwidth
#define TCP_WND                     (4 * TCP_MSS)  // Smaller window
#define TCP_SND_BUF                 (2 * TCP_MSS)  // Smaller send buffer
```

### **Connection Reuse**
```c
// Reuse TLS connections when possible
// Keep pcb alive between requests to same server
// Only create new connection if needed
```

### **Timeout Configuration**
```c
// Adjust timeouts for your network conditions
#define DEFAULT_TIMEOUT_MS          15000   // 15 seconds for slow networks
#define DNS_TIMEOUT_MS              5000    // 5 seconds for DNS
```

---

## 🔧 Common Build Issues

### **Header Conflicts with PNGdec**
**Problem**: `#define local static` in PNGdec conflicts with lwIP
**Solution**: Use C wrapper pattern:
```c
// https_c_wrapper.c - isolate all lwIP headers here
#include "lwip/altcp_tls.h"
#include "mbedtls/ssl.h"

// No lwIP headers in any C++ files!
```

### **Library Linking Order**
**Problem**: Undefined symbols for mbedTLS functions
**Solution**: Correct CMakeLists.txt order:
```cmake
target_link_libraries(${NAME}
    pico_cyw43_arch_lwip_poll     # WiFi + lwIP
    pico_lwip_mbedtls            # lwIP mbedTLS integration
    pico_mbedtls                 # mbedTLS library
)
```

### **Missing Include Directories**
**Problem**: Cannot find lwIP headers
**Solution**: Add include directory:
```cmake
target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
```

---

## 🎯 Testing Checklist

### **✅ Pre-Flight Checklist**
- [ ] WiFi connects and gets IP address
- [ ] DNS resolution works (`nslookup api.open-meteo.com`)
- [ ] Basic TCP connection possible
- [ ] TLS context creation succeeds
- [ ] SNI hostname set successfully
- [ ] Certificate verification disabled
- [ ] TLS version configured

### **✅ Connection Success Indicators**
- [ ] `SNI hostname set to: [hostname]` in logs
- [ ] `TLS handshake completed successfully!` message
- [ ] HTTP request sent without errors
- [ ] Response data received
- [ ] Connection closed cleanly

### **✅ Error Recovery Testing**
- [ ] Network disconnection handling
- [ ] DNS failure recovery
- [ ] TLS timeout handling
- [ ] Memory exhaustion recovery
- [ ] Invalid certificate handling

---

## 🚀 Production Deployment

### **Security Considerations**
- SNI provides hostname verification (prevents basic MITM)
- TLS 1.2+ encryption protects data in transit
- Certificate verification disabled (accept this risk for embedded IoT)
- Consider certificate pinning for critical applications

### **Monitoring & Logging**
```c
// Production logging - less verbose but still useful
printf("TLS connection to %s: %s\n", hostname, success ? "SUCCESS" : "FAILED");
if (!success) {
    printf("Error: %s\n", error_string);
}
```

### **Retry Strategy**
```c
// Exponential backoff for failed connections
int retry_delay_ms = 1000;  // Start with 1 second
for (int attempt = 0; attempt < 5; attempt++) {
    if (try_https_connection()) {
        break;  // Success!
    }
    sleep_ms(retry_delay_ms);
    retry_delay_ms *= 2;  // 1s, 2s, 4s, 8s, 16s
}
```

---

## 🎉 Success Pattern Summary

**The working pattern that solved ALTCP error -15:**

1. **✅ SNI Support** - `mbedtls_ssl_set_hostname()` via `altcp_tls_context()`
2. **✅ Certificate Verification Disabled** - `MBEDTLS_SSL_VERIFY_NONE` for embedded
3. **✅ TLS 1.2 Minimum** - `mbedtls_ssl_conf_min_version()` for compatibility  
4. **✅ C Wrapper Pattern** - Isolate lwIP headers from C++ code
5. **✅ Proper Library Linking** - Correct order in CMakeLists.txt
6. **✅ Enhanced Error Handling** - Comprehensive error code mapping

**Result**: Successful HTTPS connections to modern servers! 🔐✨