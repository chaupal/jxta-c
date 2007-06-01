/* 
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called "JXTA",
 *    nor may "JXTA" appear in their name, without prior written
 *    permission of Sun.
 *
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Project JXTA.  For more
 * information on Project JXTA, please see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id: jxta_tls_config_adv.c,v 1.2 2007/04/24 23:46:42 mmx2005 Exp $
 */

static const char *const __log_cat = "TlsCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_tls_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_TlsConfigAdvertisement_,
    Certificate_,
    PrivateKey_,
    CertChain_,
    Signature_,
    File_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_TlsConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;

    JString *certificate_str;
    X509 *certificate;
    JString *private_key_str;
    JString *algorithm;
    JString *salt;
    JString *ca_chain_file;
    JString *signature;
    int format;
};

   /* Forward decl. of un-exported function */
static void jxta_TlsConfigAdvertisement_delete(Jxta_object *);


/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_TlsConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:TlsConfig\n");

    ad->format = TLSCONF_CERT_FORMAT_PEM;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "format")) {
            if (strcmp(atts[1], "JXTA") == 0)
                ad->format = TLSCONF_CERT_FORMAT_JXTA;
        }
        atts += 2;
    }
}

static void handleCertificate(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    if (0 == len) {
        ad->certificate_str = NULL;
        return;
    }
    ad->certificate_str = jstring_new_2(cd);
    jstring_trim(ad->certificate_str);
}

static void handleCertChain(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;
}

static void handleFile(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    if (0 == len) {
        JXTA_OBJECT_RELEASE(ad->ca_chain_file);
        return;
    }
    ad->ca_chain_file = jstring_new_2(cd);
    jstring_trim(ad->ca_chain_file);
}

static void handleSignature(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    if (0 == len) {
        JXTA_OBJECT_RELEASE(ad->signature);
        return;
    }
    ad->signature = jstring_new_2(cd);
    jstring_trim(ad->signature);
}

static void handlePrivateKey(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    if (0 == len) {
        ad->private_key_str = NULL;
        return;
    }
    ad->private_key_str = jstring_new_2(cd);
    jstring_trim(ad->private_key_str);

    while (atts && *atts) {
        if (0 == strcmp(*atts, "algorithm")) {
            ad->algorithm = jstring_new_2(atts[1]);
            jstring_trim(ad->algorithm);
        }
        if (0 == strcmp(*atts, "salt")) {
            ad->salt = jstring_new_2(atts[1]);
            jstring_trim(ad->salt);
        }
        atts += 2;
    }
}

JXTA_DECLARE(X509 *) jxta_TLSConfigAdvertisement_get_Certificate(Jxta_TlsConfigAdvertisement * ad)
{
    if ((ad->certificate == NULL) && (ad->certificate_str != NULL)) {
        JString *cert = NULL;

        BIO *in = NULL;
        X509 *x509 = NULL;

        if (ad->format == TLSCONF_CERT_FORMAT_JXTA) {
            cert = jstring_new_0();

            jstring_append_2(cert, "-----BEGIN CERTIFICATE-----\n");
            jstring_append_1(cert, ad->certificate_str);
            jstring_append_2(cert, "\n-----END CERTIFICATE-----");
        } else {
            cert = JXTA_OBJECT_SHARE(ad->certificate_str);
        }

        in = BIO_new_mem_buf(jstring_get_string(cert), jstring_length(cert));

        ad->certificate = PEM_read_bio_X509(in, NULL, NULL, NULL);

        JXTA_OBJECT_RELEASE(cert);

        BIO_free(in);
    }

    return ad->certificate;
}

static Jxta_status jxta_TLSConfigAdvertisement_createSignature(Jxta_TlsConfigAdvertisement * ad, EVP_PKEY * p_key)
{
    if ((ad->signature == NULL) & (p_key != NULL)) {
        JString *ca_file = jxta_TLSConfigAdvertisement_get_CAChainFile(ad);

        if (ca_file != NULL) {
            void *data = NULL;
            int size = 0;

            {
                apr_pool_t *pool = NULL;
                apr_file_t *file = NULL;
                apr_finfo_t *info = malloc(sizeof(apr_finfo_t));

                apr_pool_create(&pool, NULL);

                apr_file_open(&file, jstring_get_string(ca_file), APR_READ, APR_OS_DEFAULT, pool);

                apr_file_info_get(info, APR_FINFO_SIZE, file);
                size = info->size + 2;

                data = malloc(size);

                apr_file_read(file, data, &size);

                apr_file_close(file);

                free(info);
                apr_pool_destroy(pool);
            }

            {
                EVP_MD_CTX *evp_ctx = NULL;
                BIGNUM *bn_sig = NULL;
                unsigned char *bin_sig = NULL;
                char *sig = NULL;
                unsigned int sig_size;

                evp_ctx = EVP_MD_CTX_create();

                sig_size = EVP_PKEY_size(p_key);

                bin_sig = malloc(sig_size);

                EVP_SignInit(evp_ctx, EVP_md5());
                EVP_SignUpdate(evp_ctx, data, size);

                if (EVP_SignFinal(evp_ctx, bin_sig, &sig_size, p_key) != 1) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "ChainFile-Signature wrong!!\n");

                    ERR_print_errors_fp(stderr);

                    return JXTA_FAILED;
                }

                bn_sig = BN_new();

                BN_bin2bn(bin_sig, sig_size, bn_sig);

                sig = BN_bn2hex(bn_sig);

                ad->signature = jstring_new_2(sig);

                BN_free(bn_sig);
                EVP_MD_CTX_destroy(evp_ctx);

                free(bin_sig);

                JXTA_OBJECT_RELEASE(sig);
            }
        }
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(EVP_PKEY *)
    jxta_TLSConfigAdvertisement_get_PrivateKey(Jxta_TlsConfigAdvertisement * ad, const char *pwd)
{
    EVP_PKEY *p_key = NULL;

    if (ad->private_key_str != NULL) {
        JString *key = NULL;
        BIO *in = NULL;

        if ((ad->format == TLSCONF_CERT_FORMAT_JXTA) && (ad->algorithm != NULL) && (ad->salt != NULL)) {
            key = jstring_new_0();

            jstring_append_2(key, "-----BEGIN RSA PRIVATE KEY-----\n");
            jstring_append_2(key, "Proc-Type: 4,ENCRYPTED\n");
            jstring_append_2(key, "DEK-Info: ");
            jstring_append_1(key, ad->algorithm);
            jstring_append_2(key, ",");
            jstring_append_1(key, ad->salt);
            jstring_append_2(key, "\n\n");
            jstring_append_1(key, ad->private_key_str);
            jstring_append_2(key, "\n-----END RSA PRIVATE KEY-----");
        } else {
            key = JXTA_OBJECT_SHARE(ad->private_key_str);
        }

        in = BIO_new_mem_buf(jstring_get_string(key), jstring_length(key));

        p_key = PEM_read_bio_PrivateKey(in, NULL, NULL, pwd);

        /*if (ad->signature == NULL)
           jxta_TLSConfigAdvertisement_createSignature(ad, p_key); */

        JXTA_OBJECT_RELEASE(key);

        BIO_free(in);
    }

    return p_key;
}

JXTA_DECLARE(JString *)
    jxta_TLSConfigAdvertisement_get_CAChainFile(Jxta_TlsConfigAdvertisement * ad)
{
    return JXTA_OBJECT_SHARE(ad->ca_chain_file);
}

JXTA_DECLARE(JString *)
    jxta_TLSConfigAdvertisement_get_Signature(Jxta_TlsConfigAdvertisement * ad)
{
    return JXTA_OBJECT_SHARE(ad->signature);
}

JXTA_DECLARE(JString *)
    jxta_TLSConfigAdvertisement_get_Format(Jxta_TlsConfigAdvertisement * ad)
{
    JString *rv;

    rv = jstring_new_0();

    if (ad->format == TLSCONF_CERT_FORMAT_PEM)
        jstring_append_2(rv, "PEM");
    else
        jstring_append_2(rv, "JXTA");

    return rv;
}

/** Now, build an array of the keyword structs.  Since 
* a top-level, or null state may be of interest, 
* let that lead off.  Then, walk through the enums,
* initializing the struct array with the correct fields.
* Later, the stream will be dispatched to the handler based
* on the value in the char * kwd.
*/

static const Kwdtab Jxta_TlsConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:TlsConfig", Jxta_TlsConfigAdvertisement_, *handleJxta_TlsConfigAdvertisement, NULL, NULL},
    {"Certificate", Certificate_, *handleCertificate, NULL, NULL},
    {"PrivateKey", PrivateKey_, *handlePrivateKey, NULL, NULL},
    {"CertChain", CertChain_, *handleCertChain, NULL, NULL},
    {"File", File_, *handleFile, NULL, NULL},
    {"Signature", Signature_, *handleSignature, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_TlsConfigAdvertisement_get_xml(Jxta_TlsConfigAdvertisement * ad, JString ** result)
{
    JString *string = jstring_new_0();
    jstring_append_2(string, "<!-- JXTA TLS Configuration Advertisement; keys in PEM-format -->\n");
    jstring_append_2(string, "<jxta:TlsConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:TlsConfig\" format=");
    if (ad->format == TLSCONF_CERT_FORMAT_PEM)
        jstring_append_2(string, "\"PEM\">\n");
    else
        jstring_append_2(string, "\"JXTA\">\n");

    jstring_append_2(string, "<Certificate>\n");
    if (ad->certificate_str != NULL)
        jstring_append_1(string, ad->certificate_str);
    jstring_append_2(string, "\n</Certificate>\n");

    if (ad->format == TLSCONF_CERT_FORMAT_PEM) {
        jstring_append_2(string, "<PrivateKey>\n");
        if (ad->private_key_str != NULL)
            jstring_append_1(string, ad->private_key_str);
        jstring_append_2(string, "\n</PrivateKey>\n");
    } else {
        jstring_append_2(string, "<PrivateKey algorithm=\"");
        if (ad->algorithm != NULL);
        jstring_append_1(string, ad->algorithm);
        jstring_append_2(string, "\" salt=\"");
        if (ad->salt != NULL);
        jstring_append_1(string, ad->salt);
        jstring_append_2(string, "\">\n");
        if (ad->private_key_str != NULL)
            jstring_append_1(string, ad->private_key_str);
        jstring_append_2(string, "\n</PrivateKey>\n");
    }

    if (ad->ca_chain_file != NULL) {
        JString *sig = jxta_TLSConfigAdvertisement_get_Signature(ad);

        jstring_append_2(string, "<CertChain>\n");
        jstring_append_2(string, "<Signature>\n");
        if (sig != NULL)
            jstring_append_1(string, sig);
        jstring_append_2(string, "\n</Signature>\n");
        jstring_append_2(string, "<File>");
        jstring_append_1(string, ad->ca_chain_file);
        jstring_append_2(string, "</File>\n");
        jstring_append_2(string, "</CertChain>\n");

        JXTA_OBJECT_RELEASE(sig);
    }

    jstring_append_2(string, "</jxta:TlsConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_TlsConfigAdvertisement *jxta_TlsConfigAdvertisement_construct(Jxta_TlsConfigAdvertisement * self)
{
    self = (Jxta_TlsConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:TlsConfig",
                                     Jxta_TlsConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_TlsConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {

    }
    return self;
}

void jxta_TlsConfigAdvertisement_destruct(Jxta_TlsConfigAdvertisement * self)
{
    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_TlsConfigAdvertisement *) jxta_TlsConfigAdvertisement_new(void)
{
    Jxta_TlsConfigAdvertisement *ad = (Jxta_TlsConfigAdvertisement *) calloc(1, sizeof(Jxta_TlsConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_TlsConfigAdvertisement_delete, 0);

    return jxta_TlsConfigAdvertisement_construct(ad);
}

static void jxta_TlsConfigAdvertisement_delete(Jxta_object * ad)
{
    Jxta_TlsConfigAdvertisement *myself = (Jxta_TlsConfigAdvertisement *) ad;

    jxta_TlsConfigAdvertisement_destruct((Jxta_TlsConfigAdvertisement *) ad);

    JXTA_OBJECT_RELEASE(myself->certificate_str);
    JXTA_OBJECT_RELEASE(myself->private_key_str);
    JXTA_OBJECT_RELEASE(myself->algorithm);
    JXTA_OBJECT_RELEASE(myself->salt);
    JXTA_OBJECT_RELEASE(myself->ca_chain_file);

    memset(ad, 0xDD, sizeof(Jxta_TlsConfigAdvertisement));
    free(ad);
}

/* vim: set ts=4 sw=4 et tw=130: */
