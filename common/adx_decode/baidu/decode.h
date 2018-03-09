/* Baidu decode */
#ifndef DECODE_H_
#define DECODE_H_
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include "../../baselib/base64.h"
#include "../../baselib/algo_aes.h"

extern "C"
{
int DecodeWinningPrice(char *encodedprice, double *value);
}
#endif //DECODE_H_
