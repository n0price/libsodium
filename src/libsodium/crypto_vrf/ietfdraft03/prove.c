#include <string.h>
#include <stdlib.h>

#include "crypto_hash_sha512.h"
#include "crypto_vrf.h"
#include "crypto_vrf_ietfdraft03.h"
#include "crypto_core_ed25519.h"
#include "private/ed25519_ref10.h"
#include "utils.h"


int
crypto_vrf_ietfdraft03_prove(unsigned char *proof,
                             const unsigned char *sk,
                             const unsigned char *m, unsigned long long mlen)
{

    crypto_hash_sha512_state hs;
    unsigned char az[64], r_string[64];
    unsigned char H_string[32];
    unsigned char kB_string[32], kH_string[32];
    unsigned char hram[64], nonce[64];
    ge25519_p3    H, Gamma, kB, kH;

    crypto_hash_sha512(az, sk, 32);
    az[0] &= 248;
    az[31] &= 127;
    az[31] |= 64;

    crypto_hash_sha512_init(&hs);
    crypto_hash_sha512_update(&hs, &SUITE, 1);
    crypto_hash_sha512_update(&hs, &ONE, 1);
    crypto_hash_sha512_update(&hs, sk + 32, 32);
    crypto_hash_sha512_update(&hs, m, mlen);
    crypto_hash_sha512_final(&hs, r_string);

    r_string[31] &= 0x7f; /* clear sign bit */
    ge25519_from_uniform(H_string, r_string); /* elligator2 */

    ge25519_frombytes(&H, H_string);
    ge25519_scalarmult(&Gamma, az, &H);

    crypto_hash_sha512_init(&hs);
    crypto_hash_sha512_update(&hs, az + 32, 32);
    crypto_hash_sha512_update(&hs, H_string, 32);
    crypto_hash_sha512_final(&hs, nonce);

    sc25519_reduce(nonce);
    ge25519_scalarmult_base(&kB, nonce);
    ge25519_scalarmult(&kH, nonce, &H);

    ge25519_p3_tobytes(proof, &Gamma);
    ge25519_p3_tobytes(kB_string, &kB);
    ge25519_p3_tobytes(kH_string, &kH);

    crypto_hash_sha512_init(&hs);
    crypto_hash_sha512_update(&hs, &SUITE, 1);
    crypto_hash_sha512_update(&hs, &TWO, 1);
    crypto_hash_sha512_update(&hs, H_string, 32);
    crypto_hash_sha512_update(&hs, proof, 32);
    crypto_hash_sha512_update(&hs, kB_string, 32);
    crypto_hash_sha512_update(&hs, kH_string, 32);
    crypto_hash_sha512_final(&hs, hram);

    memmove(proof + 32, hram, 16);
    memset(hram + 16, 0, 48); /* we zero out the last 48 bytes of the challenge */
    sc25519_muladd(proof + 48, hram, az, nonce);

    sodium_memzero(az, sizeof az);
    sodium_memzero(nonce, sizeof nonce);

    return 0;
}
