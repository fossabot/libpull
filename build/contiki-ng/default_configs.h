#ifndef DEFAULT_CONFIGS_H_
#define DEFAULT_CONFIGS_H_

static uint8_t x[32] = {
    0x8b,0x27,0x39,0x67,0x01,0x4b,0x1c,0xae,0xfe,0x8a,0x18,0x6e,0xea,0x27,0x86,0x34,
    0x0e,0xea,0x35,0x3d,0x8c,0x65,0xf6,0x59,0xfc,0xcb,0x23,0xd7,0xfa,0xab,0x7b,0x18
};
static uint8_t y[32] = {
    0x14,0x75,0x33,0xec,0x17,0xb7,0x54,0x50,0xca,0x98,0x35,0xad,0x58,0xbe,0xd5,0xfa,
    0x48,0xbc,0xa0,0x24,0x81,0xba,0xfa,0x3d,0xcd,0x8d,0x5a,0x7f,0x40,0xbc,0x70,0x94
}
;
#if defined(CONN_DTLS_ECDSA) || defined(CONN_DTLS_PSK)

#include "coap-dtls.h"

#ifdef CONN_DTLS_ECDSA
int verify_key(uint8_t key_size,
        const uint8_t* server_pub_x, const uint8_t* server_pub_y) {
    /* TODO implement server validation */
    return 1;
}
static uint8_t priv[32] = {
    0xf7,0xfd,0xc9,0x84,0x7a,0x1f,0xb2,0xac,
    0xed,0xc4,0x22,0x72,0x3b,0x88,0x5d,0x35,
    0xca,0x62,0x8c,0x51,0xce,0x41,0xe4,0x38,
    0xf0,0x64,0xe2,0xce,0xa5,0x9d,0xef,0x49
};
static uint8_t pubx[32] = {
    0x8b,0x27,0x39,0x67,0x01,0x4b,0x1c,0xae,
    0xfe,0x8a,0x18,0x6e,0xea,0x27,0x86,0x34,
    0x0e,0xea,0x35,0x3d,0x8c,0x65,0xf6,0x59,
    0xfc,0xcb,0x23,0xd7,0xfa,0xab,0x7b,0x18
};
static uint8_t puby[32] = {
    0x14,0x75,0x33,0xec,0x17,0xb7,0x54,0x50,
    0xca,0x98,0x35,0xad,0x58,0xbe,0xd5,0xfa,
    0x48,0xbc,0xa0,0x24,0x81,0xba,0xfa,0x3d,
    0xcd,0x8d,0x5a,0x7f,0x40,0xbc,0x70,0x94
};

static dtls_ecdsa_data_t dtls_ecdsa_data = {
    .curve = DTLS_CURVE_SECP256R1,
    .priv_key = priv,
    .pub_key_x = pubx,
    .pub_key_y = puby,
    .verify_key = verify_key
};

#endif /* CONN_DTLS_ECDSA */

#ifdef CONN_DTLS_PSK
static dtls_psk_data_t dtls_psk_data = {
    .hint = "CoAP",
    .hint_len = 4,
    .key = "ThisIsOurSecret1",
    .key_len = 16
};
#endif /* CONN_DTLS_PSK */

#endif

#endif /* DEFAULT_CONFIGS_H_ */

