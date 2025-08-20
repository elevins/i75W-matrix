#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

// Basic mbedtls configuration for Pico W networking
// This is a minimal configuration for HTTP client use

// Enable basic cryptographic features
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_STD_CALLOC calloc
#define MBEDTLS_PLATFORM_STD_FREE free

// Basic cipher support
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C

// Hash functions
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C

// Big number support
#define MBEDTLS_BIGNUM_C

// Random number generation
#define MBEDTLS_ENTROPY_HARDWARE_ALT

// Memory allocation
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C

// Disable features we don't need for basic HTTP
#undef MBEDTLS_RSA_C
#undef MBEDTLS_PK_C
#undef MBEDTLS_X509_CRT_PARSE_C
#undef MBEDTLS_SSL_TLS_C

// Platform specific
#if defined(__ARMEL__)
#define MBEDTLS_HAVE_ASM
#endif

// Include the default config
#include "mbedtls/mbedtls_config.h"

#endif /* MBEDTLS_CONFIG_H */