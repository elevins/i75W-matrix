#ifndef MBEDTLS_USER_CONFIG_FILE
#define MBEDTLS_USER_CONFIG_FILE

// Include the default mbedTLS config first
#include "mbedtls/mbedtls_config.h"

// Override settings for Pico W embedded system
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C

// Reduce memory usage
#define MBEDTLS_SSL_MAX_CONTENT_LEN 4096

// Basic networking support  
#define MBEDTLS_NET_C
#define MBEDTLS_TIMING_C

#endif /* MBEDTLS_USER_CONFIG_FILE */