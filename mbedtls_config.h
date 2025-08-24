#ifndef MBEDTLS_USER_CONFIG_FILE
#define MBEDTLS_USER_CONFIG_FILE

// Minimal mbedTLS configuration for lwIP compatibility
// Based on lwIP's requirements and mbedTLS 3.6 API

// System support
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS

// SSL/TLS Core
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C

// Session Management - Minimal implementation
#define MBEDTLS_SSL_SESSION_C

// Cryptographic algorithms
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_GCM
#define MBEDTLS_GCM_C
#define MBEDTLS_CIPHER_C

// Hash algorithms
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_MD_C

// Public key cryptography
#define MBEDTLS_RSA_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C

// Certificate support
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_OID_C
#define MBEDTLS_BASE64_C

// Key exchange
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PKCS1_V21

// Random number generation
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

// Network support - not needed for embedded lwIP integration
// #define MBEDTLS_NET_C  // Disabled - Unix/Windows only

// Protocol versions
#define MBEDTLS_SSL_PROTO_TLS1_2

// Features
#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_ERROR_C

// Disable problematic features
#define MBEDTLS_SSL_SESSION_TICKETS 0
#define MBEDTLS_USE_PSA_CRYPTO 0

// Memory settings
#define MBEDTLS_SSL_MAX_CONTENT_LEN 4096

#endif /* MBEDTLS_USER_CONFIG_FILE */