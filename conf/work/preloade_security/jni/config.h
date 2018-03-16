
#ifndef WOLF_CONFIG_H
#define WOLF_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WOLFSSL_KEY_GEN
#define WOLFSSL_KEY_GEN
#endif

#ifndef WOLFSSL_CERT_GEN
#define WOLFSSL_CERT_GEN
#endif

#ifndef WOLFSSL_CERT_EXT
#define WOLFSSL_CERT_EXT
#endif

#ifndef WOLFSSL_PUB_PEM_TO_DER
#define WOLFSSL_PUB_PEM_TO_DER
#endif

#ifndef WOLFSSL_ALT_NAMES
#define WOLFSSL_ALT_NAMES
#endif

#ifndef WOLFSSL_CERT_REQ
#define WOLFSSL_CERT_REQ
#endif

#ifdef __cplusplus
}
#endif


#endif /* WOLFSSL_OPTIONS_H */
