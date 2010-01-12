/*
* SHA-{384,512}
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/sha2_64.h>
#include <botan/loadstor.h>
#include <botan/rotate.h>

namespace Botan {

namespace {

/*
* SHA-{384,512} Rho Function
*/
inline u64bit rho(u64bit X, u32bit rot1, u32bit rot2, u32bit rot3)
   {
   return (rotate_right(X, rot1) ^ rotate_right(X, rot2) ^
           rotate_right(X, rot3));
   }

/*
* SHA-{384,512} F1 Function
*/
inline void F1(u64bit A, u64bit B, u64bit C, u64bit& D,
               u64bit E, u64bit F, u64bit G, u64bit& H,
               u64bit msg, u64bit magic)
   {
   magic += rho(E, 14, 18, 41) + ((E & F) ^ (~E & G)) + msg;
   D += magic + H;
   H += magic + rho(A, 28, 34, 39) + ((A & B) ^ (A & C) ^ (B & C));
   }

/*
* SHA-{384,512} Sigma Function
*/
inline u64bit sigma(u64bit X, u32bit rot1, u32bit rot2, u32bit shift)
   {
   return (rotate_right(X, rot1) ^ rotate_right(X, rot2) ^ (X >> shift));
   }

}

/*
* SHA-{384,512} Compression Function
*/
void SHA_384_512_BASE::compress_n(const byte input[], u32bit blocks)
   {
   u64bit A = digest[0], B = digest[1], C = digest[2],
          D = digest[3], E = digest[4], F = digest[5],
          G = digest[6], H = digest[7];

   for(u32bit i = 0; i != blocks; ++i)
      {
      for(u32bit j = 0; j != 16; ++j)
         W[j] = load_be<u64bit>(input, j);
      input += HASH_BLOCK_SIZE;

      for(u32bit j = 16; j != 80; ++j)
         W[j] = sigma(W[j- 2], 19, 61,  6) + W[j- 7] +
                sigma(W[j-15],  1,  8,  7) + W[j-16];

      F1(A, B, C, D, E, F, G, H, W[ 0], (u64bit) 0x428A2F98D728AE22ULL);
      F1(H, A, B, C, D, E, F, G, W[ 1], (u64bit) 0x7137449123EF65CDULL);
      F1(G, H, A, B, C, D, E, F, W[ 2], (u64bit) 0xB5C0FBCFEC4D3B2FULL);
      F1(F, G, H, A, B, C, D, E, W[ 3], (u64bit) 0xE9B5DBA58189DBBCULL);
      F1(E, F, G, H, A, B, C, D, W[ 4], (u64bit) 0x3956C25BF348B538ULL);
      F1(D, E, F, G, H, A, B, C, W[ 5], (u64bit) 0x59F111F1B605D019ULL);
      F1(C, D, E, F, G, H, A, B, W[ 6], (u64bit) 0x923F82A4AF194F9BULL);
      F1(B, C, D, E, F, G, H, A, W[ 7], (u64bit) 0xAB1C5ED5DA6D8118ULL);
      F1(A, B, C, D, E, F, G, H, W[ 8], (u64bit) 0xD807AA98A3030242ULL);
      F1(H, A, B, C, D, E, F, G, W[ 9], (u64bit) 0x12835B0145706FBEULL);
      F1(G, H, A, B, C, D, E, F, W[10], (u64bit) 0x243185BE4EE4B28CULL);
      F1(F, G, H, A, B, C, D, E, W[11], (u64bit) 0x550C7DC3D5FFB4E2ULL);
      F1(E, F, G, H, A, B, C, D, W[12], (u64bit) 0x72BE5D74F27B896FULL);
      F1(D, E, F, G, H, A, B, C, W[13], (u64bit) 0x80DEB1FE3B1696B1ULL);
      F1(C, D, E, F, G, H, A, B, W[14], (u64bit) 0x9BDC06A725C71235ULL);
      F1(B, C, D, E, F, G, H, A, W[15], (u64bit) 0xC19BF174CF692694ULL);
      F1(A, B, C, D, E, F, G, H, W[16], (u64bit) 0xE49B69C19EF14AD2ULL);
      F1(H, A, B, C, D, E, F, G, W[17], (u64bit) 0xEFBE4786384F25E3ULL);
      F1(G, H, A, B, C, D, E, F, W[18], (u64bit) 0x0FC19DC68B8CD5B5ULL);
      F1(F, G, H, A, B, C, D, E, W[19], (u64bit) 0x240CA1CC77AC9C65ULL);
      F1(E, F, G, H, A, B, C, D, W[20], (u64bit) 0x2DE92C6F592B0275ULL);
      F1(D, E, F, G, H, A, B, C, W[21], (u64bit) 0x4A7484AA6EA6E483ULL);
      F1(C, D, E, F, G, H, A, B, W[22], (u64bit) 0x5CB0A9DCBD41FBD4ULL);
      F1(B, C, D, E, F, G, H, A, W[23], (u64bit) 0x76F988DA831153B5ULL);
      F1(A, B, C, D, E, F, G, H, W[24], (u64bit) 0x983E5152EE66DFABULL);
      F1(H, A, B, C, D, E, F, G, W[25], (u64bit) 0xA831C66D2DB43210ULL);
      F1(G, H, A, B, C, D, E, F, W[26], (u64bit) 0xB00327C898FB213FULL);
      F1(F, G, H, A, B, C, D, E, W[27], (u64bit) 0xBF597FC7BEEF0EE4ULL);
      F1(E, F, G, H, A, B, C, D, W[28], (u64bit) 0xC6E00BF33DA88FC2ULL);
      F1(D, E, F, G, H, A, B, C, W[29], (u64bit) 0xD5A79147930AA725ULL);
      F1(C, D, E, F, G, H, A, B, W[30], (u64bit) 0x06CA6351E003826FULL);
      F1(B, C, D, E, F, G, H, A, W[31], (u64bit) 0x142929670A0E6E70ULL);
      F1(A, B, C, D, E, F, G, H, W[32], (u64bit) 0x27B70A8546D22FFCULL);
      F1(H, A, B, C, D, E, F, G, W[33], (u64bit) 0x2E1B21385C26C926ULL);
      F1(G, H, A, B, C, D, E, F, W[34], (u64bit) 0x4D2C6DFC5AC42AEDULL);
      F1(F, G, H, A, B, C, D, E, W[35], (u64bit) 0x53380D139D95B3DFULL);
      F1(E, F, G, H, A, B, C, D, W[36], (u64bit) 0x650A73548BAF63DEULL);
      F1(D, E, F, G, H, A, B, C, W[37], (u64bit) 0x766A0ABB3C77B2A8ULL);
      F1(C, D, E, F, G, H, A, B, W[38], (u64bit) 0x81C2C92E47EDAEE6ULL);
      F1(B, C, D, E, F, G, H, A, W[39], (u64bit) 0x92722C851482353BULL);
      F1(A, B, C, D, E, F, G, H, W[40], (u64bit) 0xA2BFE8A14CF10364ULL);
      F1(H, A, B, C, D, E, F, G, W[41], (u64bit) 0xA81A664BBC423001ULL);
      F1(G, H, A, B, C, D, E, F, W[42], (u64bit) 0xC24B8B70D0F89791ULL);
      F1(F, G, H, A, B, C, D, E, W[43], (u64bit) 0xC76C51A30654BE30ULL);
      F1(E, F, G, H, A, B, C, D, W[44], (u64bit) 0xD192E819D6EF5218ULL);
      F1(D, E, F, G, H, A, B, C, W[45], (u64bit) 0xD69906245565A910ULL);
      F1(C, D, E, F, G, H, A, B, W[46], (u64bit) 0xF40E35855771202AULL);
      F1(B, C, D, E, F, G, H, A, W[47], (u64bit) 0x106AA07032BBD1B8ULL);
      F1(A, B, C, D, E, F, G, H, W[48], (u64bit) 0x19A4C116B8D2D0C8ULL);
      F1(H, A, B, C, D, E, F, G, W[49], (u64bit) 0x1E376C085141AB53ULL);
      F1(G, H, A, B, C, D, E, F, W[50], (u64bit) 0x2748774CDF8EEB99ULL);
      F1(F, G, H, A, B, C, D, E, W[51], (u64bit) 0x34B0BCB5E19B48A8ULL);
      F1(E, F, G, H, A, B, C, D, W[52], (u64bit) 0x391C0CB3C5C95A63ULL);
      F1(D, E, F, G, H, A, B, C, W[53], (u64bit) 0x4ED8AA4AE3418ACBULL);
      F1(C, D, E, F, G, H, A, B, W[54], (u64bit) 0x5B9CCA4F7763E373ULL);
      F1(B, C, D, E, F, G, H, A, W[55], (u64bit) 0x682E6FF3D6B2B8A3ULL);
      F1(A, B, C, D, E, F, G, H, W[56], (u64bit) 0x748F82EE5DEFB2FCULL);
      F1(H, A, B, C, D, E, F, G, W[57], (u64bit) 0x78A5636F43172F60ULL);
      F1(G, H, A, B, C, D, E, F, W[58], (u64bit) 0x84C87814A1F0AB72ULL);
      F1(F, G, H, A, B, C, D, E, W[59], (u64bit) 0x8CC702081A6439ECULL);
      F1(E, F, G, H, A, B, C, D, W[60], (u64bit) 0x90BEFFFA23631E28ULL);
      F1(D, E, F, G, H, A, B, C, W[61], (u64bit) 0xA4506CEBDE82BDE9ULL);
      F1(C, D, E, F, G, H, A, B, W[62], (u64bit) 0xBEF9A3F7B2C67915ULL);
      F1(B, C, D, E, F, G, H, A, W[63], (u64bit) 0xC67178F2E372532BULL);
      F1(A, B, C, D, E, F, G, H, W[64], (u64bit) 0xCA273ECEEA26619CULL);
      F1(H, A, B, C, D, E, F, G, W[65], (u64bit) 0xD186B8C721C0C207ULL);
      F1(G, H, A, B, C, D, E, F, W[66], (u64bit) 0xEADA7DD6CDE0EB1EULL);
      F1(F, G, H, A, B, C, D, E, W[67], (u64bit) 0xF57D4F7FEE6ED178ULL);
      F1(E, F, G, H, A, B, C, D, W[68], (u64bit) 0x06F067AA72176FBAULL);
      F1(D, E, F, G, H, A, B, C, W[69], (u64bit) 0x0A637DC5A2C898A6ULL);
      F1(C, D, E, F, G, H, A, B, W[70], (u64bit) 0x113F9804BEF90DAEULL);
      F1(B, C, D, E, F, G, H, A, W[71], (u64bit) 0x1B710B35131C471BULL);
      F1(A, B, C, D, E, F, G, H, W[72], (u64bit) 0x28DB77F523047D84ULL);
      F1(H, A, B, C, D, E, F, G, W[73], (u64bit) 0x32CAAB7B40C72493ULL);
      F1(G, H, A, B, C, D, E, F, W[74], (u64bit) 0x3C9EBE0A15C9BEBCULL);
      F1(F, G, H, A, B, C, D, E, W[75], (u64bit) 0x431D67C49C100D4CULL);
      F1(E, F, G, H, A, B, C, D, W[76], (u64bit) 0x4CC5D4BECB3E42B6ULL);
      F1(D, E, F, G, H, A, B, C, W[77], (u64bit) 0x597F299CFC657E2AULL);
      F1(C, D, E, F, G, H, A, B, W[78], (u64bit) 0x5FCB6FAB3AD6FAECULL);
      F1(B, C, D, E, F, G, H, A, W[79], (u64bit) 0x6C44198C4A475817ULL);

      A = (digest[0] += A);
      B = (digest[1] += B);
      C = (digest[2] += C);
      D = (digest[3] += D);
      E = (digest[4] += E);
      F = (digest[5] += F);
      G = (digest[6] += G);
      H = (digest[7] += H);
      }
   }

/*
* Copy out the digest
*/
void SHA_384_512_BASE::copy_out(byte output[])
   {
   for(u32bit j = 0; j != OUTPUT_LENGTH; j += 8)
      store_be(digest[j/8], output + j);
   }

/*
* Clear memory of sensitive data
*/
void SHA_384_512_BASE::clear() throw()
   {
   MDx_HashFunction::clear();
   W.clear();
   }

/*
* Clear memory of sensitive data
*/
void SHA_384::clear() throw()
   {
   SHA_384_512_BASE::clear();
   digest[0] = (u64bit) 0xCBBB9D5DC1059ED8ULL;
   digest[1] = (u64bit) 0x629A292A367CD507ULL;
   digest[2] = (u64bit) 0x9159015A3070DD17ULL;
   digest[3] = (u64bit) 0x152FECD8F70E5939ULL;
   digest[4] = (u64bit) 0x67332667FFC00B31ULL;
   digest[5] = (u64bit) 0x8EB44A8768581511ULL;
   digest[6] = (u64bit) 0xDB0C2E0D64F98FA7ULL;
   digest[7] = (u64bit) 0x47B5481DBEFA4FA4ULL;
   }

/*
* Clear memory of sensitive data
*/
void SHA_512::clear() throw()
   {
   SHA_384_512_BASE::clear();
   digest[0] = (u64bit) 0x6A09E667F3BCC908ULL;
   digest[1] = (u64bit) 0xBB67AE8584CAA73BULL;
   digest[2] = (u64bit) 0x3C6EF372FE94F82BULL;
   digest[3] = (u64bit) 0xA54FF53A5F1D36F1ULL;
   digest[4] = (u64bit) 0x510E527FADE682D1ULL;
   digest[5] = (u64bit) 0x9B05688C2B3E6C1FULL;
   digest[6] = (u64bit) 0x1F83D9ABFB41BD6BULL;
   digest[7] = (u64bit) 0x5BE0CD19137E2179ULL;
   }

}
