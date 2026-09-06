/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */
/* Further modifications to include the "UNRAVEL" stuff, below */

/* This code is in the public domain */

#include "edsio.h"
#include <string.h>

#define SHA_BLOCKSIZE           64
#define SHA_DIGESTSIZE          20

/* UNRAVEL should be fastest & biggest */
/* UNROLL_LOOPS should be just as big, but slightly slower */
/* both undefined should be smallest and slowest */

#define UNRAVEL
/* #define UNROLL_LOOPS */

/* by default, compile for little-endian machines (Intel, Vax) */
/* change for big-endian machines; for machines which are neither, */
/* you will need to change the definition of maybe_byte_reverse */

#ifndef WORDS_BIGENDIAN /* from config.h */
#define SHA_LITTLE_ENDIAN
#endif

/* NIST's proposed modification to SHA of 7/11/94 may be */
/* activated by defining USE_MODIFIED_SHA; leave it off for now */
#undef USE_MODIFIED_SHA

/* SHA f()-functions */

#define f1(x,y,z)       ((x & y) | (~x & z))
#define f2(x,y,z)       (x ^ y ^ z)
#define f3(x,y,z)       ((x & y) | (x & z) | (y & z))
#define f4(x,y,z)       (x ^ y ^ z)

/* SHA constants */

#define CONST1          0x5a827999L
#define CONST2          0x6ed9eba1L
#define CONST3          0x8f1bbcdcL
#define CONST4          0xca62c1d6L

/* 32-bit rotate */

#define ROT32(x,n)      ((x << n) | (x >> (32 - n)))

/* the generic case, for when the overall rotation is not unraveled */

#define FG(n)   \
    T = ROT32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n;        \
    E = D; D = C; C = ROT32(B,30); B = A; A = T

/* specific cases, for when the overall rotation is unraveled */

#define FA(n)   \
    T = ROT32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n; B = ROT32(B,30)

#define FB(n)   \
    E = ROT32(T,5) + f##n(A,B,C) + D + *WP++ + CONST##n; A = ROT32(A,30)

#define FC(n)   \
    D = ROT32(E,5) + f##n(T,A,B) + C + *WP++ + CONST##n; T = ROT32(T,30)

#define FD(n)   \
    C = ROT32(D,5) + f##n(E,T,A) + B + *WP++ + CONST##n; E = ROT32(E,30)

#define FE(n)   \
    B = ROT32(C,5) + f##n(D,E,T) + A + *WP++ + CONST##n; D = ROT32(D,30)

#define FT(n)   \
    A = ROT32(B,5) + f##n(C,D,E) + T + *WP++ + CONST##n; C = ROT32(C,30)

/* do SHA transformation */

static void sha_transform(EdsioSHACtx *ctx)
{
    int i;
    guint32 T, A, B, C, D, E, W[80], *WP;

    for (i = 0; i < 16; ++i) {
        W[i] = ctx->data[i];
    }
    for (i = 16; i < 80; ++i) {
        W[i] = W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16];
#ifdef USE_MODIFIED_SHA
        W[i] = ROT32(W[i], 1);
#endif /* USE_MODIFIED_SHA */
    }
    A = ctx->digest[0];
    B = ctx->digest[1];
    C = ctx->digest[2];
    D = ctx->digest[3];
    E = ctx->digest[4];
    WP = W;
#ifdef UNRAVEL
    FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1); FC(1); FD(1);
    FE(1); FT(1); FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1);
    FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2); FE(2); FT(2);
    FA(2); FB(2); FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2);
    FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3); FA(3); FB(3);
    FC(3); FD(3); FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3);
    FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4); FC(4); FD(4);
    FE(4); FT(4); FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4);
    ctx->digest[0] += E;
    ctx->digest[1] += T;
    ctx->digest[2] += A;
    ctx->digest[3] += B;
    ctx->digest[4] += C;
#else /* !UNRAVEL */
#ifdef UNROLL_LOOPS
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
#else /* !UNROLL_LOOPS */
    for (i =  0; i < 20; ++i) { FG(1); }
    for (i = 20; i < 40; ++i) { FG(2); }
    for (i = 40; i < 60; ++i) { FG(3); }
    for (i = 60; i < 80; ++i) { FG(4); }
#endif /* !UNROLL_LOOPS */
    ctx->digest[0] += A;
    ctx->digest[1] += B;
    ctx->digest[2] += C;
    ctx->digest[3] += D;
    ctx->digest[4] += E;
#endif /* !UNRAVEL */
}

#ifdef SHA_LITTLE_ENDIAN

/* change endianness of data */

static void maybe_byte_reverse(guint32 *buffer, int count)
{
    int i;
    guint32 in;

    count /= sizeof(guint32);
    for (i = 0; i < count; ++i) {
        in = *buffer;
        *buffer++ = ((in << 24) & 0xff000000) | ((in <<  8) & 0x00ff0000) |
                    ((in >>  8) & 0x0000ff00) | ((in >> 24) & 0x000000ff);
    }
}

#else /* !SHA_LITTLE_ENDIAN */

#define maybe_byte_reverse(a,b) /* do nothing */

#endif /* SHA_LITTLE_ENDIAN */

/* initialize the SHA digest */

void edsio_sha_init(EdsioSHACtx *ctx)
{
    ctx->digest[0] = 0x67452301L;
    ctx->digest[1] = 0xefcdab89L;
    ctx->digest[2] = 0x98badcfeL;
    ctx->digest[3] = 0x10325476L;
    ctx->digest[4] = 0xc3d2e1f0L;
    ctx->count_lo = 0L;
    ctx->count_hi = 0L;
    ctx->local = 0;
}

/* update the SHA digest */

void edsio_sha_update(EdsioSHACtx *ctx, const guint8 *buffer, guint count)
{
    int i;

    if ((ctx->count_lo + ((guint32) count << 3)) < ctx->count_lo) {
        ++ctx->count_hi;
    }
    ctx->count_lo += (guint32) count << 3;
    ctx->count_hi += (guint32) count >> 29;
    if (ctx->local) {
        i = SHA_BLOCKSIZE - ctx->local;
        if (i > count) {
            i = count;
        }
        memcpy(((guint8 *) ctx->data) + ctx->local, buffer, i);
        count -= i;
        buffer += i;
        ctx->local += i;
        if (ctx->local == SHA_BLOCKSIZE) {
            maybe_byte_reverse(ctx->data, SHA_BLOCKSIZE);
            sha_transform(ctx);
        } else {
            return;
        }
    }
    while (count >= SHA_BLOCKSIZE) {
        memcpy(ctx->data, buffer, SHA_BLOCKSIZE);
        buffer += SHA_BLOCKSIZE;
        count -= SHA_BLOCKSIZE;
        maybe_byte_reverse(ctx->data, SHA_BLOCKSIZE);
        sha_transform(ctx);
    }
    memcpy(ctx->data, buffer, count);
    ctx->local = count;
}

/* finish computing the SHA digest */

void edsio_sha_final(guint8* digest, EdsioSHACtx *ctx)
{
    int count;
    guint32 lo_bit_count, hi_bit_count;

    lo_bit_count = ctx->count_lo;
    hi_bit_count = ctx->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x3f);
    ((guint8 *) ctx->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
        memset(((guint8 *) ctx->data) + count, 0, SHA_BLOCKSIZE - count);
        maybe_byte_reverse(ctx->data, SHA_BLOCKSIZE);
        sha_transform(ctx);
        memset((guint8 *) ctx->data, 0, SHA_BLOCKSIZE - 8);
    } else {
        memset(((guint8 *) ctx->data) + count, 0,
            SHA_BLOCKSIZE - 8 - count);
    }
    maybe_byte_reverse(ctx->data, SHA_BLOCKSIZE);
    ctx->data[14] = hi_bit_count;
    ctx->data[15] = lo_bit_count;
    sha_transform(ctx);

    memcpy (digest, ctx->digest, 20);
}
