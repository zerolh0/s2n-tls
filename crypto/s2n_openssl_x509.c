/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "crypto/s2n_openssl_x509.h"

#include "api/s2n.h"

S2N_CLEANUP_RESULT s2n_openssl_x509_stack_pop_free(STACK_OF(X509) **cert_chain)
{
    RESULT_ENSURE_REF(*cert_chain);
    sk_X509_pop_free(*cert_chain, X509_free);
    *cert_chain = NULL;
    return S2N_RESULT_OK;
}

S2N_CLEANUP_RESULT s2n_openssl_asn1_time_free_pointer(ASN1_GENERALIZEDTIME **time_ptr)
{
    /* The ANS1_*TIME structs are just typedef wrappers around ASN1_STRING
     *
     * The ASN1_TIME, ASN1_UTCTIME and ASN1_GENERALIZEDTIME structures are
     * represented as an ASN1_STRING internally and can be freed up using
     * ASN1_STRING_free().
     * https://www.openssl.org/docs/man1.1.1/man3/ASN1_TIME_to_tm.html
     */
    RESULT_ENSURE_REF(*time_ptr);
    ASN1_STRING_free((ASN1_STRING *) *time_ptr);
    *time_ptr = NULL;
    return S2N_RESULT_OK;
}

S2N_RESULT s2n_openssl_x509_parse_impl(struct s2n_blob *asn1der, X509 **cert_out, uint32_t *parsed_length)
{
    RESULT_ENSURE_REF(asn1der);
    RESULT_ENSURE_REF(asn1der->data);
    RESULT_ENSURE_REF(cert_out);
    RESULT_ENSURE_REF(parsed_length);

    uint8_t *cert_to_parse = asn1der->data;
    *cert_out = d2i_X509(NULL, (const unsigned char **) (void *) &cert_to_parse, asn1der->size);
    RESULT_ENSURE(*cert_out != NULL, S2N_ERR_DECODE_CERTIFICATE);

    /* If cert parsing is successful, d2i_X509 increments *cert_to_parse to the byte following the parsed data */
    *parsed_length = cert_to_parse - asn1der->data;

    return S2N_RESULT_OK;
}

S2N_RESULT s2n_openssl_x509_parse_without_length_validation(struct s2n_blob *asn1der, X509 **cert_out)
{
    RESULT_ENSURE_REF(asn1der);
    RESULT_ENSURE_REF(cert_out);

    uint32_t parsed_len = 0;
    RESULT_GUARD(s2n_openssl_x509_parse_impl(asn1der, cert_out, &parsed_len));

    return S2N_RESULT_OK;
}

S2N_RESULT s2n_openssl_x509_parse(struct s2n_blob *asn1der, X509 **cert_out)
{
    RESULT_ENSURE_REF(asn1der);
    RESULT_ENSURE_REF(cert_out);

    uint32_t parsed_len = 0;
    RESULT_GUARD(s2n_openssl_x509_parse_impl(asn1der, cert_out, &parsed_len));

    /* Some TLS clients in the wild send extra trailing bytes after the Certificate.
     * Allow this in s2n for backwards compatibility with existing clients. */
    uint32_t trailing_bytes = asn1der->size - parsed_len;
    RESULT_ENSURE(trailing_bytes <= S2N_MAX_ALLOWED_CERT_TRAILING_BYTES, S2N_ERR_DECODE_CERTIFICATE);

    return S2N_RESULT_OK;
}
