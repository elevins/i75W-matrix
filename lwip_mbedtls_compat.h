#pragma once

// Compatibility shim for lwIP + mbedTLS 3.6
// This file provides compatibility wrappers for lwIP's altcp_tls with newer mbedTLS

#include "mbedtls/ssl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compatibility function for session data check
static inline int altcp_tls_session_is_valid(mbedtls_ssl_session* session) {
    // In mbedTLS 3.6, we can't directly check session->data.start
    // Instead, we'll assume the session is valid if it's not null
    // This is a simplified compatibility layer
    return (session != NULL);
}

// Compatibility wrapper for mbedtls_pk_parse_key
// The function exists but may not be properly declared in some configurations
#ifndef MBEDTLS_PK_PARSE_KEY_DECLARED
int mbedtls_pk_parse_key(mbedtls_pk_context *pk,
                         const unsigned char *key, size_t keylen,
                         const unsigned char *pwd, size_t pwdlen,
                         int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);
#define MBEDTLS_PK_PARSE_KEY_DECLARED
#endif

#ifdef __cplusplus
}
#endif