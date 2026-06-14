#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_MD_SHA256 6

typedef struct mbedtls_md_context_t {
    int dummy;
} mbedtls_md_context_t;

typedef struct mbedtls_md_type_t {
    int type;
} mbedtls_md_type_t;

typedef struct mbedtls_md_info_t {
    int dummy;
} mbedtls_md_info_t;

static inline void mbedtls_md_init(mbedtls_md_context_t *ctx) { (void)ctx; }
static inline void mbedtls_md_free(mbedtls_md_context_t *ctx) { (void)ctx; }
static inline int mbedtls_md_setup(mbedtls_md_context_t *ctx, const mbedtls_md_info_t *info, int hmac) {
    (void)ctx; (void)info; (void)hmac; return 0;
}
static inline int mbedtls_md_hmac_starts(mbedtls_md_context_t *ctx, const unsigned char *key, size_t keylen) {
    (void)ctx; (void)key; (void)keylen; return 0;
}
static inline int mbedtls_md_hmac_update(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen) {
    (void)ctx; (void)input; (void)ilen; return 0;
}
static inline int mbedtls_md_hmac_finish(mbedtls_md_context_t *ctx, unsigned char *output) {
    (void)ctx; (void)output; return 0;
}
static inline const mbedtls_md_info_t *mbedtls_md_info_from_type(int md_type) {
    (void)md_type; return (const mbedtls_md_info_t*)1;
}

// Alias for older API naming (starts/update/finish without hmac_ prefix)
static inline int mbedtls_md_starts(mbedtls_md_context_t *ctx) { (void)ctx; return 0; }
static inline int mbedtls_md_update(mbedtls_md_context_t *ctx, const unsigned char *input, size_t ilen) {
    (void)ctx; (void)input; (void)ilen; return 0;
}
static inline int mbedtls_md_finish(mbedtls_md_context_t *ctx, unsigned char *output) {
    (void)ctx; (void)output; return 0;
}

#ifdef __cplusplus
}
#endif
