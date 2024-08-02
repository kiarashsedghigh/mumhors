/* LibTomCrypt, modular cryptographic library -- Tom St Denis
*
* LibTomCrypt is a library that provides various cryptographic
* algorithms in a highly modular and flexible manner.
*
* The library is free for all purposes without any express
* guarantee it works.
*/

/*
  BLAKE2 reference source code package - reference C implementations

  Copyright 2012, Samuel Neves <sneves@dei.uc.pt>.  You may use this under the
  terms of the CC0, the OpenSSL Licence, or the Apache Public License 2.0, at
  your option.  The terms of these licenses can be found at:

  - CC0 1.0 Universal : http://creativecommons.org/publicdomain/zero/1.0
  - OpenSSL license   : https://www.openssl.org/source/license.html
  - Apache 2.0        : http://www.apache.org/licenses/LICENSE-2.0

  More information about the BLAKE2 hash function can be found at
  https://blake2.net.
*/
/* see also https://www.ietf.org/rfc/rfc7693.txt */

#include "tomcrypt.h"

#ifdef LTC_BLAKE2S

enum blake2s_constant {
 BLAKE2S_BLOCKBYTES = 64,
 BLAKE2S_OUTBYTES = 32,
 BLAKE2S_KEYBYTES = 32,
 BLAKE2S_SALTBYTES = 8,
 BLAKE2S_PERSONALBYTES = 8,
 BLAKE2S_PARAM_SIZE = 32
};

/* param offsets */
enum {
 O_DIGEST_LENGTH = 0,
 O_KEY_LENGTH = 1,
 O_FANOUT = 2,
 O_DEPTH = 3,
 O_LEAF_LENGTH = 4,
 O_NODE_OFFSET = 8,
 O_XOF_LENGTH = 12,
 O_NODE_DEPTH = 14,
 O_INNER_LENGTH = 15,
 O_SALT = 16,
 O_PERSONAL = 24
};

/*
struct blake2s_param {
  unsigned char digest_length;
  unsigned char key_length;
  unsigned char fanout;
  unsigned char depth;
  ulong32 leaf_length;
  ulong32 node_offset;
  ushort16 xof_length;
  unsigned char node_depth;
  unsigned char inner_length;
  unsigned char salt[BLAKE2S_SALTBYTES];
  unsigned char personal[BLAKE2S_PERSONALBYTES];
};
*/

const struct ltc_hash_descriptor blake2s_128_desc =
   {
       "blake2s-128",
       21,
       16,
       64,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 2, 4 },
       11,
       &blake2s_128_init,
       &blake2s_process,
       &blake2s_done,
       NULL, //&blake2s_128_test,
       NULL
};

const struct ltc_hash_descriptor blake2s_160_desc =
   {
       "blake2s-160",
       22,
       20,
       64,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 2, 5 },
       11,
       &blake2s_160_init,
       &blake2s_process,
       &blake2s_done,
       NULL, //&blake2s_160_test,
       NULL
};

const struct ltc_hash_descriptor blake2s_224_desc =
   {
       "blake2s-224",
       23,
       28,
       64,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 2, 7 },
       11,
       &blake2s_224_init,
       &blake2s_process,
       &blake2s_done,
       NULL, //&blake2s_224_test,
       NULL
};

const struct ltc_hash_descriptor blake2s_256_desc =
   {
       "blake2s-256",
       24,
       32,
       64,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 2, 8 },
       11,
       &blake2s_256_init,
       &blake2s_process,
       &blake2s_done,
       NULL, //&blake2s_256_test,
       NULL
};

static const ulong32 blake2s_IV[8] = {
   0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL,
   0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL
};

static const unsigned char blake2s_sigma[10][16] = {
   { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
   { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
   { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
   { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
   { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
   { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
   { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
   { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
   { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
   { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
};

static void blake2s_set_lastnode(hash_state *md) { md->blake2s.f[1] = 0xffffffffUL; }

/* Some helper functions, not necessarily useful */
static int blake2s_is_lastblock(const hash_state *md) { return md->blake2s.f[0] != 0; }

static void blake2s_set_lastblock(hash_state *md)
{
 if (md->blake2s.last_node)
   blake2s_set_lastnode(md);

 md->blake2s.f[0] = 0xffffffffUL;
}

static void blake2s_increment_counter(hash_state *md, const ulong32 inc)
{
 md->blake2s.t[0] += inc;
 if (md->blake2s.t[0] < inc) md->blake2s.t[1]++;
}

static int blake2s_init0(hash_state *md)
{
 int i;
 XMEMSET(&md->blake2s, 0, sizeof(struct blake2s_state));

 for (i = 0; i < 8; ++i)
   md->blake2s.h[i] = blake2s_IV[i];

 return CRYPT_OK;
}

/* init2 xors IV with input parameter block */
static int blake2s_init_param(hash_state *md, const unsigned char *P)
{
 unsigned long i;

 blake2s_init0(md);

 /* IV XOR ParamBlock */
 for (i = 0; i < 8; ++i) {
   ulong32 tmp;
   LOAD32L(tmp, P + i * 4);
   md->blake2s.h[i] ^= tmp;
 }

 md->blake2s.outlen = P[O_DIGEST_LENGTH];
 return CRYPT_OK;
}

int blake2s_init(hash_state *md, unsigned long outlen, const unsigned char *key, unsigned long keylen)
{
 unsigned char P[BLAKE2S_PARAM_SIZE];
 int err;

 LTC_ARGCHK(md != NULL);

 if ((!outlen) || (outlen > BLAKE2S_OUTBYTES))
   return CRYPT_INVALID_ARG;

 if ((key && !keylen) || (keylen && !key) || (keylen > BLAKE2S_KEYBYTES))
   return CRYPT_INVALID_ARG;

 XMEMSET(P, 0, sizeof(P));

 P[O_DIGEST_LENGTH] = (unsigned char)outlen;
 P[O_KEY_LENGTH] = (unsigned char)keylen;
 P[O_FANOUT] = 1;
 P[O_DEPTH] = 1;

 err = blake2s_init_param(md, P);
 if (err != CRYPT_OK) return err;

 if (key) {
   unsigned char block[BLAKE2S_BLOCKBYTES];

   XMEMSET(block, 0, BLAKE2S_BLOCKBYTES);
   XMEMCPY(block, key, keylen);
   blake2s_process(md, block, BLAKE2S_BLOCKBYTES);

#ifdef LTC_CLEAN_STACK
   zeromem(block, sizeof(block));
#endif
 }
 return CRYPT_OK;
}

int blake2s_128_init(hash_state *md) { return blake2s_init(md, 16, NULL, 0); }

int blake2s_160_init(hash_state *md) { return blake2s_init(md, 20, NULL, 0); }

int blake2s_224_init(hash_state *md) { return blake2s_init(md, 28, NULL, 0); }

int blake2s_256_init(hash_state *md) { return blake2s_init(md, 32, NULL, 0); }

#define G(r, i, a, b, c, d)                                                                                            \
  do {                                                                                                                \
     a = a + b + m[blake2s_sigma[r][2 * i + 0]];                                                                      \
     d = ROR(d ^ a, 16);                                                                                              \
     c = c + d;                                                                                                       \
     b = ROR(b ^ c, 12);                                                                                              \
     a = a + b + m[blake2s_sigma[r][2 * i + 1]];                                                                      \
     d = ROR(d ^ a, 8);                                                                                               \
     c = c + d;                                                                                                       \
     b = ROR(b ^ c, 7);                                                                                               \
  } while (0)
#define ROUND(r)                                                                                                       \
  do {                                                                                                                \
     G(r, 0, v[0], v[4], v[8], v[12]);                                                                                \
     G(r, 1, v[1], v[5], v[9], v[13]);                                                                                \
     G(r, 2, v[2], v[6], v[10], v[14]);                                                                               \
     G(r, 3, v[3], v[7], v[11], v[15]);                                                                               \
     G(r, 4, v[0], v[5], v[10], v[15]);                                                                               \
     G(r, 5, v[1], v[6], v[11], v[12]);                                                                               \
     G(r, 6, v[2], v[7], v[8], v[13]);                                                                                \
     G(r, 7, v[3], v[4], v[9], v[14]);                                                                                \
  } while (0)

#ifdef LTC_CLEAN_STACK
static int _blake2s_compress(hash_state *md, const unsigned char *buf)
#else
static int blake2s_compress(hash_state *md, const unsigned char *buf)
#endif
{
 unsigned long i;
 ulong32 m[16];
 ulong32 v[16];

 for (i = 0; i < 16; ++i) {
   LOAD32L(m[i], buf + i * sizeof(m[i]));
 }

 for (i = 0; i < 8; ++i)
   v[i] = md->blake2s.h[i];

 v[8] = blake2s_IV[0];
 v[9] = blake2s_IV[1];
 v[10] = blake2s_IV[2];
 v[11] = blake2s_IV[3];
 v[12] = md->blake2s.t[0] ^ blake2s_IV[4];
 v[13] = md->blake2s.t[1] ^ blake2s_IV[5];
 v[14] = md->blake2s.f[0] ^ blake2s_IV[6];
 v[15] = md->blake2s.f[1] ^ blake2s_IV[7];

 ROUND(0);
 ROUND(1);
 ROUND(2);
 ROUND(3);
 ROUND(4);
 ROUND(5);
 ROUND(6);
 ROUND(7);
 ROUND(8);
 ROUND(9);

 for (i = 0; i < 8; ++i)
   md->blake2s.h[i] = md->blake2s.h[i] ^ v[i] ^ v[i + 8];

 return CRYPT_OK;
}
#undef G
#undef ROUND

#ifdef LTC_CLEAN_STACK
static int blake2s_compress(hash_state *md, const unsigned char *buf)
{
 int err;
 err = _blake2s_compress(md, buf);
 burn_stack(sizeof(ulong32) * (32) + sizeof(unsigned long));
 return err;
}
#endif

int blake2s_process(hash_state *md, const unsigned char *in, unsigned long inlen)
{
 LTC_ARGCHK(md != NULL);
 LTC_ARGCHK(in != NULL);

 if (md->blake2s.curlen > sizeof(md->blake2s.buf)) {
   return CRYPT_INVALID_ARG;
 }

 if (inlen > 0) {
   unsigned long left = md->blake2s.curlen;
   unsigned long fill = BLAKE2S_BLOCKBYTES - left;
   if (inlen > fill) {
     md->blake2s.curlen = 0;
     XMEMCPY(md->blake2s.buf + (left % sizeof(md->blake2s.buf)), in, fill); /* Fill buffer */
     blake2s_increment_counter(md, BLAKE2S_BLOCKBYTES);
     blake2s_compress(md, md->blake2s.buf); /* Compress */
     in += fill;
     inlen -= fill;
     while (inlen > BLAKE2S_BLOCKBYTES) {
       blake2s_increment_counter(md, BLAKE2S_BLOCKBYTES);
       blake2s_compress(md, in);
       in += BLAKE2S_BLOCKBYTES;
       inlen -= BLAKE2S_BLOCKBYTES;
     }
   }
   XMEMCPY(md->blake2s.buf + md->blake2s.curlen, in, inlen);
   md->blake2s.curlen += inlen;
 }
 return CRYPT_OK;
}

int blake2s_done(hash_state *md, unsigned char *out)
{
 unsigned char buffer[BLAKE2S_OUTBYTES] = { 0 };
 unsigned long i;

 LTC_ARGCHK(md != NULL);
 LTC_ARGCHK(out != NULL);

 /* if(md->blake2s.outlen != outlen) return CRYPT_INVALID_ARG; */

 if (blake2s_is_lastblock(md))
   return CRYPT_ERROR;

 blake2s_increment_counter(md, md->blake2s.curlen);
 blake2s_set_lastblock(md);
 XMEMSET(md->blake2s.buf + md->blake2s.curlen, 0, BLAKE2S_BLOCKBYTES - md->blake2s.curlen); /* Padding */
 blake2s_compress(md, md->blake2s.buf);

 for (i = 0; i < 8; ++i) /* Output full hash to temp buffer */
   STORE32L(md->blake2s.h[i], buffer + i * 4);

 XMEMCPY(out, buffer, md->blake2s.outlen);
 zeromem(md, sizeof(hash_state));
#ifdef LTC_CLEAN_STACK
 zeromem(buffer, sizeof(buffer));
#endif
 return CRYPT_OK;
}

#endif


#define BLAKE2s_128 16
#define BLAKE2s_160 20
#define BLAKE2s_224 28
#define BLAKE2s_256 32

int blake2s_128(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;
  blake2s_128_init(&md);
  blake2s_process(&md, input, length);
  blake2s_done(&md, hash_output);

  return BLAKE2s_128;
}


int blake2s_160(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;

  blake2s_160_init(&md);
  blake2s_process(&md, input, length);
  blake2s_done(&md, hash_output);

  return BLAKE2s_160;
}


int blake2s_224(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;

  blake2s_224_init(&md);
  blake2s_process(&md, input, length);
  blake2s_done(&md, hash_output);

  return BLAKE2s_224;
}

int blake2s_256(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;

  blake2s_256_init(&md);
  blake2s_process(&md, input, length);
  blake2s_done(&md, hash_output);

  return BLAKE2s_256;
}



