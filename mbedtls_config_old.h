#ifndef MBEDTLS_USER_CONFIG_FILE
#define MBEDTLS_USER_CONFIG_FILE

// Don't include default config, define everything ourselves

// Core features
#define MBEDTLS_CONFIG_VERSION 0x03060000

// Override settings for Pico W embedded system
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS

// Reduce memory usage
#define MBEDTLS_SSL_MAX_CONTENT_LEN 4096

// Basic networking support  
#define MBEDTLS_NET_C
#define MBEDTLS_TIMING_C

// Basic defines
#define MBEDTLS_HAVE_ASM

// SSL/TLS support
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C

// Key exchange
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21

// Cryptographic primitives
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_CCM
#define MBEDTLS_GCM_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_MD_C

// Certificate support
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_OID_C

// Random number generation for Pico hardware
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C

// Bignum operations
#define MBEDTLS_BIGNUM_C

// Base64 decoding for certificates
#define MBEDTLS_BASE64_C

// Cipher suites
#define MBEDTLS_CIPHER_C

// PK support for private keys
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PK_RSA_ALT_SUPPORT

// Compatibility defines for lwIP
#define MBEDTLS_SSL_SESSION_C
#define MBEDTLS_SSL_CONTEXT_C

// Disable problematic features for compatibility
#define MBEDTLS_SSL_SESSION_TICKETS 0

// Enable compatibility features
#define MBEDTLS_USE_PSA_CRYPTO 0
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_ALPN
#define MBEDTLS_SSL_SERVER_NAME_INDICATION

// Error handling
#define MBEDTLS_ERROR_C

#endif /* MBEDTLS_USER_CONFIG_FILE */