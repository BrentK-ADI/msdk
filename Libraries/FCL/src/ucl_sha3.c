/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by 
 * Analog Devices, Inc.),
 * Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

//SHA3 implementation
//this implementation used the implementation from Markku-Juhani O.Saarinen
//as a starting point

#include <ucl/ucl_hash.h>
#ifdef HASH_SHA3
#include <string.h>
#include <ucl/ucl_config.h>
#include <ucl/ucl_retdefs.h>
#include <ucl/ucl_types.h>
#include <ucl/ucl_sha3.h>
#include <ucl/ucl_sys.h>

#define N_ROUNDS 24 //the specialization for keccak-f from keccak-p

#define ROTL64(x, y) (((x) << (y)) | ((x) >> ((sizeof(u64)*8) - (y))))

const u64 kcf_rc[24] = {0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000, 0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009, 0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a, 0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a, 0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008};

static const u8 kcf_rho[24] = {1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14, 27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44};

static const u8 kcf_pilane[24] = {10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4, 15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1};

// generally called after SHA3_SPONGE_WORDS-ctx->capacityWords words
 // are XORed into the state s
static void kcf(u64 state[25])
{
    int i, j, round;
    u64 t, c[5];

    //I(chi(Pi(ro(theta(
    for (round = 0; round < N_ROUNDS; round++) {
        // Theta
        for (i = 0; i < 5; i++) {
            c[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }

        for (i = 0; i < 5; i++) {
            t = c[(i + 4) % 5] ^ (u64)ROTL64(c[(i + 1) % 5], 1);

            for (j = 0; j < 25; j += 5) {
                state[j + i] ^= t;
            }
        }

        // Rho Pi
        t = state[1];
        for (i = 0; i < 24; i++) {
            j = (int)kcf_pilane[i];
            c[0] = state[j];
            state[j] = (u64)ROTL64(t, kcf_rho[i]);
            t = c[0];
        }

        // Chi
        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++) {
                c[i] = state[j + i];
            }

            for (i = 0; i < 5; i++) {
                state[j + i] ^= (~c[(i + 1) % 5]) & c[(i + 2) % 5];
            }
        }

        // Iota
        state[0] ^= kcf_rc[round];
    }
}

int ucl_shake128_init(ucl_sha3_ctx_t *ctx)
{
    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 2 * 128 / (8 * sizeof(u64));

    return(UCL_OK);
}

int ucl_sha3_224_init(ucl_sha3_ctx_t *ctx)
{
    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 448 / (8 * sizeof(u64));

    return(UCL_OK);
}

int ucl_sha3_256_init(ucl_sha3_ctx_t *ctx)
{
    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 512 / (8 * sizeof(u64));

    return(UCL_OK);
}

int ucl_shake256_init(ucl_sha3_ctx_t *ctx)
{
    return(ucl_sha3_256_init(ctx));
}

int ucl_sha3_384_init(ucl_sha3_ctx_t *ctx)
{
    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 768 / (8 * sizeof(u64));

    return(UCL_OK);
}

int ucl_sha3_512_init(ucl_sha3_ctx_t *ctx)
{
    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->capacityWords = 1024 / (8 * sizeof(u64));

    return(UCL_OK);
}

int ucl_sha3_core(ucl_sha3_ctx_t *ctx, const u8 *bufIn, u32 len)
{
    u32 old_tail = (8 - ctx->byteIndex) & 7;
    size_t words;
    int tail;
    size_t i;
    const u8 *buf = bufIn;

    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == bufIn) {
        return(UCL_INVALID_INPUT);
    }

    if (len < old_tail) {
        while (len--) {
            ctx->saved |= (u64) (*(buf++)) << ((ctx->byteIndex++) * 8);
        }

        return(UCL_OK);
    }

    if (old_tail) {
        len -= old_tail;
        while (old_tail--)
        ctx->saved |= (u64) (*(buf++)) << ((ctx->byteIndex++) * 8);
        ctx->s[ctx->wordIndex] ^= ctx->saved;
        ctx->byteIndex = 0;
        ctx->saved = 0;
        if (++ctx->wordIndex == ((int)SHA3_SPONGE_WORDS - ctx->capacityWords)) {
            kcf(ctx->s);
            ctx->wordIndex = 0;
        }
    }

    words = len / sizeof(u64);
    tail = (int)(len - words * sizeof(u64));

    for (i = 0; i < words; i++, buf += sizeof(u64)) {
        const u64 t = (u64) (buf[0]) | ((u64) (buf[1]) << 8 * 1) | ((u64) (buf[2]) << 8 * 2) | ((u64) (buf[3]) << 8 * 3) | ((u64) (buf[4]) << 8 * 4) | ((u64) (buf[5]) << 8 * 5) | ((u64) (buf[6]) << 8 * 6) | ((u64) (buf[7]) << 8 * 7);
        ctx->s[ctx->wordIndex] ^= t;
        if (++ctx->wordIndex == ((int)SHA3_SPONGE_WORDS - ctx->capacityWords)){
            kcf(ctx->s);
            ctx->wordIndex = 0;
        }
    }

    while (tail--) {
        ctx->saved |= (u64) (*(buf++)) << ((ctx->byteIndex++) * 8);
    }

    return(UCL_OK);
}

int ucl_sha3_finish(u8 *digest, ucl_sha3_ctx_t *ctx)
{
    // SHA3 version
    ctx->s[ctx->wordIndex] ^= (ctx->saved ^ ((u64) ((u64) (0x02 | (1 << 2)) << ((ctx->byteIndex) * 8))));
    ctx->s[(int)SHA3_SPONGE_WORDS - ctx->capacityWords - 1] ^= (u64)0x8000000000000000UL;
    kcf(ctx->s);

    int i;

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    for (i = 0; i < (int)SHA3_SPONGE_WORDS; i++) {
        const u32 t1 = (u32) ctx->s[i];
        const u32 t2 = (u32) ((ctx->s[i] >> 16) >> 16);
        ctx->sb[i * 8 + 0] = (u8) (t1);
        ctx->sb[i * 8 + 1] = (u8) (t1 >> 8);
        ctx->sb[i * 8 + 2] = (u8) (t1 >> 16);
        ctx->sb[i * 8 + 3] = (u8) (t1 >> 24);
        ctx->sb[i * 8 + 4] = (u8) (t2);
        ctx->sb[i * 8 + 5] = (u8) (t2 >> 8);
        ctx->sb[i * 8 + 6] = (u8) (t2 >> 16);
        ctx->sb[i * 8 + 7] = (u8) (t2 >> 24);
    }

    for (i = 0; i < (int)SHA3_SPONGE_WORDS * 8; i++) {
        digest[i] = ctx->sb[i];
    }

    return(UCL_OK);
}

int ucl_shake_finish(u8 *digest, ucl_sha3_ctx_t *ctx)
{
    int i;

    if (NULL == ctx) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    ctx->s[ctx->wordIndex] ^= (ctx->saved ^ ((u64) (0x1F) << ((ctx->byteIndex) * 8)));
    i = (int)SHA3_SPONGE_WORDS - ctx->capacityWords - 1;
    ctx->s[(int)SHA3_SPONGE_WORDS - ctx->capacityWords - 1] ^= (u64)(0x8000000000000000UL);
    kcf(ctx->s);

    for (i = 0; i < (int)SHA3_SPONGE_WORDS; i++) {
        const u32 t1 = (u32) ctx->s[i];
        const u32 t2 = (u32) ((ctx->s[i] >> 16) >> 16);
        ctx->sb[i * 8 + 0] = (u8) (t1);
        ctx->sb[i * 8 + 1] = (u8) (t1 >> 8);
        ctx->sb[i * 8 + 2] = (u8) (t1 >> 16);
        ctx->sb[i * 8 + 3] = (u8) (t1 >> 24);
        ctx->sb[i * 8 + 4] = (u8) (t2);
        ctx->sb[i * 8 + 5] = (u8) (t2 >> 8);
        ctx->sb[i * 8 + 6] = (u8) (t2 >> 16);
        ctx->sb[i * 8 + 7] = (u8) (t2 >> 24);
    }

    for (i = 0; i < (int)SHA3_SPONGE_WORDS * 8; i++) {
        digest[i] = ctx->sb[i];
    }

    return(UCL_OK);
}

int ucl_sha3_224(u8 *digest, u8 *msg, u32 msgLen)
{
    ucl_sha3_ctx_t ctx;
    if (NULL == msg) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    if (UCL_OK != ucl_sha3_224_init(&ctx)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_core(&ctx, msg, msgLen)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_finish(digest, &ctx)) {
        return(UCL_ERROR);
    }

    return(UCL_OK);
}

int ucl_sha3_256(u8 *digest, u8 *msg, u32 msgLen)
{
    ucl_sha3_ctx_t ctx;

    if (NULL == msg) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest)
        return(UCL_INVALID_OUTPUT);

    if (UCL_OK != ucl_sha3_256_init(&ctx)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_core(&ctx, msg, msgLen)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_finish(digest, &ctx)) {
        return(UCL_ERROR);
    }

    return(UCL_OK);
}

int ucl_shake128(u8 *digest, u8 *msg, u32 msgLen)
{
    ucl_sha3_ctx_t ctx;

    if (NULL == msg) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    if (UCL_OK != ucl_shake128_init(&ctx)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_core(&ctx, msg, msgLen)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_shake_finish(digest, &ctx)) {
        return(UCL_ERROR);
    }

    return(UCL_OK);
}

int ucl_sha3_384(u8 *digest, u8 *msg, u32 msgLen)
{
    ucl_sha3_ctx_t ctx;

    if (NULL == msg) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    if (UCL_OK != ucl_sha3_384_init(&ctx)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_core(&ctx, msg, msgLen)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_finish(digest, &ctx)) {
        return(UCL_ERROR);
    }

    return(UCL_OK);
}

int ucl_sha3_512(u8 *digest, u8 *msg, u32 msgLen)
{
    ucl_sha3_ctx_t ctx;
    if (NULL == msg) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    if (UCL_OK != ucl_sha3_512_init(&ctx)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_core(&ctx, msg, msgLen)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_finish(digest, &ctx)) {
        return(UCL_ERROR);
    }

    return(UCL_OK);
}

int ucl_shake256(u8 *digest, u8 *msg, u32 msgLen)
{
    ucl_sha3_ctx_t ctx;
    if (NULL == msg) {
        return(UCL_INVALID_INPUT);
    }

    if (NULL == digest) {
        return(UCL_INVALID_OUTPUT);
    }

    if (UCL_OK != ucl_shake256_init(&ctx)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_sha3_core(&ctx, msg, msgLen)) {
        return(UCL_ERROR);
    }

    if (UCL_OK != ucl_shake_finish(digest, &ctx)) {
        return(UCL_ERROR);
    }

    return(UCL_OK);
}

#endif//SHA3
