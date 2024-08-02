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

#ifdef LTC_BLAKE2B

enum blake2b_constant {
 BLAKE2B_BLOCKBYTES = 128,
 BLAKE2B_OUTBYTES = 64,
 BLAKE2B_KEYBYTES = 64,
 BLAKE2B_SALTBYTES = 16,
 BLAKE2B_PERSONALBYTES = 16,
 BLAKE2B_PARAM_SIZE = 64
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
 O_NODE_DEPTH = 16,
 O_INNER_LENGTH = 17,
 O_RESERVED = 18,
 O_SALT = 32,
 O_PERSONAL = 48
};

/*
struct blake2b_param {
  unsigned char digest_length;
  unsigned char key_length;
  unsigned char fanout;
  unsigned char depth;
  ulong32 leaf_length;
  ulong32 node_offset;
  ulong32 xof_length;
  unsigned char node_depth;
  unsigned char inner_length;
  unsigned char reserved[14];
  unsigned char salt[BLAKE2B_SALTBYTES];
  unsigned char personal[BLAKE2B_PERSONALBYTES];
};
*/

const struct ltc_hash_descriptor blake2b_160_desc =
   {
       "blake2b-160",
       25,
       20,
       128,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 1, 5 },
       11,
       &blake2b_160_init,
       &blake2b_process,
       &blake2b_done,
       NULL, //&blake2b_160_test,
       NULL
};

const struct ltc_hash_descriptor blake2b_256_desc =
   {
       "blake2b-256",
       26,
       32,
       128,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 1, 8 },
       11,
       &blake2b_256_init,
       &blake2b_process,
       &blake2b_done,
       NULL, //&blake2b_256_test,
       NULL
};

const struct ltc_hash_descriptor blake2b_384_desc =
   {
       "blake2b-384",
       27,
       48,
       128,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 1, 12 },
       11,
       &blake2b_384_init,
       &blake2b_process,
       &blake2b_done,
       NULL, //&blake2b_384_test,
       NULL
};

const struct ltc_hash_descriptor blake2b_512_desc =
   {
       "blake2b-512",
       28,
       64,
       128,
       { 1, 3, 6, 1, 4, 1, 1722, 12, 2, 1, 16 },
       11,
       &blake2b_512_init,
       &blake2b_process,
       &blake2b_done,
       NULL, //&blake2b_512_test,
       NULL
};

static const ulong64 blake2b_IV[8] =
   {
       CONST64(0x6a09e667f3bcc908), CONST64(0xbb67ae8584caa73b),
       CONST64(0x3c6ef372fe94f82b), CONST64(0xa54ff53a5f1d36f1),
       CONST64(0x510e527fade682d1), CONST64(0x9b05688c2b3e6c1f),
       CONST64(0x1f83d9abfb41bd6b), CONST64(0x5be0cd19137e2179)
};

static const unsigned char blake2b_sigma[12][16] =
   {
       {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
       { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
       { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
       {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
       {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
       {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
       { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
       { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
       {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
       { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
       {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
       { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};

static void blake2b_set_lastnode(hash_state *md) { md->blake2b.f[1] = CONST64(0xffffffffffffffff); }

/* Some helper functions, not necessarily useful */
static int blake2b_is_lastblock(const hash_state *md) { return md->blake2b.f[0] != 0; }

static void blake2b_set_lastblock(hash_state *md)
{
 if (md->blake2b.last_node)
   blake2b_set_lastnode(md);

 md->blake2b.f[0] = CONST64(0xffffffffffffffff);
}

static void blake2b_increment_counter(hash_state *md, ulong64 inc)
{
 md->blake2b.t[0] += inc;
 if (md->blake2b.t[0] < inc) md->blake2b.t[1]++;
}

static void blake2b_init0(hash_state *md)
{
 unsigned long i;
 XMEMSET(&md->blake2b, 0, sizeof(md->blake2b));

 for (i = 0; i < 8; ++i)
   md->blake2b.h[i] = blake2b_IV[i];
}

/* init xors IV with input parameter block */
static int blake2b_init_param(hash_state *md, const unsigned char *P)
{
 unsigned long i;

 blake2b_init0(md);

 /* IV XOR ParamBlock */
 for (i = 0; i < 8; ++i) {
   ulong64 tmp;
   LOAD64L(tmp, P + i * 8);
   md->blake2b.h[i] ^= tmp;
 }

 md->blake2b.outlen = P[O_DIGEST_LENGTH];
 return CRYPT_OK;
}

int blake2b_init(hash_state *md, unsigned long outlen, const unsigned char *key, unsigned long keylen)
{
 unsigned char P[BLAKE2B_PARAM_SIZE];
 int err;

 LTC_ARGCHK(md != NULL);

 if ((!outlen) || (outlen > BLAKE2B_OUTBYTES))
   return CRYPT_INVALID_ARG;

 if ((key && !keylen) || (keylen && !key) || (keylen > BLAKE2B_KEYBYTES))
   return CRYPT_INVALID_ARG;

 XMEMSET(P, 0, sizeof(P));

 P[O_DIGEST_LENGTH] = (unsigned char)outlen;
 P[O_KEY_LENGTH] = (unsigned char)keylen;
 P[O_FANOUT] = 1;
 P[O_DEPTH] = 1;

 err = blake2b_init_param(md, P);
 if (err != CRYPT_OK) return err;

 if (key) {
   unsigned char block[BLAKE2B_BLOCKBYTES];

   XMEMSET(block, 0, BLAKE2B_BLOCKBYTES);
   XMEMCPY(block, key, keylen);
   blake2b_process(md, block, BLAKE2B_BLOCKBYTES);

#ifdef LTC_CLEAN_STACK
   zeromem(block, sizeof(block));
#endif
 }

 return CRYPT_OK;
}

int blake2b_160_init(hash_state *md) { return blake2b_init(md, 20, NULL, 0); }

int blake2b_256_init(hash_state *md) { return blake2b_init(md, 32, NULL, 0); }

int blake2b_384_init(hash_state *md) { return blake2b_init(md, 48, NULL, 0); }

int blake2b_512_init(hash_state *md) { return blake2b_init(md, 64, NULL, 0); }

#define G(r, i, a, b, c, d)                                                                                            \
  do {                                                                                                                \
     a = a + b + m[blake2b_sigma[r][2 * i + 0]];                                                                      \
     d = ROR64(d ^ a, 32);                                                                                            \
     c = c + d;                                                                                                       \
     b = ROR64(b ^ c, 24);                                                                                            \
     a = a + b + m[blake2b_sigma[r][2 * i + 1]];                                                                      \
     d = ROR64(d ^ a, 16);                                                                                            \
     c = c + d;                                                                                                       \
     b = ROR64(b ^ c, 63);                                                                                            \
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
static int _blake2b_compress(hash_state *md, const unsigned char *buf)
#else
static int blake2b_compress(hash_state *md, const unsigned char *buf)
#endif
{
 ulong64 m[16];
 ulong64 v[16];
 unsigned long i;

 for (i = 0; i < 16; ++i) {
   LOAD64L(m[i], buf + i * sizeof(m[i]));
 }

 for (i = 0; i < 8; ++i) {
   v[i] = md->blake2b.h[i];
 }

 v[8] = blake2b_IV[0];
 v[9] = blake2b_IV[1];
 v[10] = blake2b_IV[2];
 v[11] = blake2b_IV[3];
 v[12] = blake2b_IV[4] ^ md->blake2b.t[0];
 v[13] = blake2b_IV[5] ^ md->blake2b.t[1];
 v[14] = blake2b_IV[6] ^ md->blake2b.f[0];
 v[15] = blake2b_IV[7] ^ md->blake2b.f[1];

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
 ROUND(10);
 ROUND(11);

 for (i = 0; i < 8; ++i) {
   md->blake2b.h[i] = md->blake2b.h[i] ^ v[i] ^ v[i + 8];
 }
 return CRYPT_OK;
}

#undef G
#undef ROUND

#ifdef LTC_CLEAN_STACK
static int blake2b_compress(hash_state *md, const unsigned char *buf)
{
 int err;
 err = _blake2b_compress(md, buf);
 burn_stack(sizeof(ulong64) * 32 + sizeof(unsigned long));
 return err;
}
#endif

int blake2b_process(hash_state *md, const unsigned char *in, unsigned long inlen)
{
 LTC_ARGCHK(md != NULL);
 LTC_ARGCHK(in != NULL);

 if (md->blake2b.curlen > sizeof(md->blake2b.buf)) {
   return CRYPT_INVALID_ARG;
 }

 if (inlen > 0) {
   unsigned long left = md->blake2b.curlen;
   unsigned long fill = BLAKE2B_BLOCKBYTES - left;
   if (inlen > fill) {
     md->blake2b.curlen = 0;
     XMEMCPY(md->blake2b.buf + (left % sizeof(md->blake2b.buf)), in, fill); /* Fill buffer */
     blake2b_increment_counter(md, BLAKE2B_BLOCKBYTES);
     blake2b_compress(md, md->blake2b.buf); /* Compress */
     in += fill;
     inlen -= fill;
     while (inlen > BLAKE2B_BLOCKBYTES) {
       blake2b_increment_counter(md, BLAKE2B_BLOCKBYTES);
       blake2b_compress(md, in);
       in += BLAKE2B_BLOCKBYTES;
       inlen -= BLAKE2B_BLOCKBYTES;
     }
   }
   XMEMCPY(md->blake2b.buf + md->blake2b.curlen, in, inlen);
   md->blake2b.curlen += inlen;
 }
 return CRYPT_OK;
}

int blake2b_done(hash_state *md, unsigned char *out)
{
 unsigned char buffer[BLAKE2B_OUTBYTES] = { 0 };
 unsigned long i;

 LTC_ARGCHK(md != NULL);
 LTC_ARGCHK(out != NULL);

 /* if(md->blakebs.outlen != outlen) return CRYPT_INVALID_ARG; */

 if (blake2b_is_lastblock(md))
   return CRYPT_ERROR;

 blake2b_increment_counter(md, md->blake2b.curlen);
 blake2b_set_lastblock(md);
 XMEMSET(md->blake2b.buf + md->blake2b.curlen, 0, BLAKE2B_BLOCKBYTES - md->blake2b.curlen); /* Padding */
 blake2b_compress(md, md->blake2b.buf);

 for (i = 0; i < 8; ++i) /* Output full hash to temp buffer */
   STORE64L(md->blake2b.h[i], buffer + i * 8);

 XMEMCPY(out, buffer, md->blake2b.outlen);
 zeromem(md, sizeof(hash_state));
#ifdef LTC_CLEAN_STACK
 zeromem(buffer, sizeof(buffer));
#endif
 return CRYPT_OK;
}


#endif


#define BLAKE2b_256 32
#define BLAKE2b_384 48
#define BLAKE2b_512 64


int blake2b_256(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;
  blake2b_256_init(&md);
  blake2b_process(&md, input, length);
  blake2b_done(&md, hash_output);

  return BLAKE2b_256;
}


int blake2b_384(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;

  blake2b_384_init(&md);
  blake2b_process(&md, input, length);
  blake2b_done(&md, hash_output);

  return BLAKE2b_384;
}


int blake2b_512(unsigned char * hash_output, const unsigned char * input , long length) {
  hash_state md;

  blake2b_512_init(&md);
  blake2b_process(&md, input, length);
  blake2b_done(&md, hash_output);

  return BLAKE2b_512;
}

