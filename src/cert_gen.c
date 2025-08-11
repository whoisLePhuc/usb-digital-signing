#define _POSIX_C_SOURCE 200809L
#include "../inc/cert_gen.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* Helper: sanitize small strings for subject components */
static void sanitize_component(const char *in, char *out, size_t outsz) {
    if (!in || !out || outsz == 0) {
        if (out && outsz>0) out[0]='\0';
        return;
    }
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 1 < outsz; ++i) {
        unsigned char c = (unsigned char)in[i];
        if (c == '/' || c == '\r' || c == '\n' || c == '"' || c == '\\') {
            out[j++] = '_';
        } else if (c >= 32 && c <= 126) {
            out[j++] = c;
        } else {
            out[j++] = '_';
        }
    }
    out[j] = '\0';
}

/* Ensure directory exists for a given path (best-effort) */
static void ensure_parent_dir(const char *path) {
    if (!path) return;
    const char *p = strrchr(path, '/');
    if (!p) return;
    size_t len = p - path;
    if (len == 0) return;
    char dir[1024];
    if (len >= sizeof(dir)) return;
    memcpy(dir, path, len);
    dir[len] = '\0';
    char cmd[1200];
    /* mkdir -p is convenient; error ignored if not available */
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s' 2>/usbInfo/null", dir);
    (void)system(cmd);
}

/* Generate EVP_PKEY RSA key of given bits */
static EVP_PKEY *generate_rsa_key_obj(int bits) {
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) return NULL;
    if (EVP_PKEY_keygen_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return NULL; }
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0) { EVP_PKEY_CTX_free(ctx); return NULL; }
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) { EVP_PKEY_CTX_free(ctx); return NULL; }
    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

int certgen_generate_key_pem(const char *key_path, int bits) {
    if (!key_path || bits < 1024) return -1;
    ensure_parent_dir(key_path);

    EVP_PKEY *pkey = generate_rsa_key_obj(bits);
    if (!pkey) {
        fprintf(stderr, "certgen: failed to generate RSA key\n");
        return -2;
    }

    FILE *f = fopen(key_path, "wb");
    if (!f) {
        perror("certgen: fopen key");
        EVP_PKEY_free(pkey);
        return -3;
    }

    int rc = PEM_write_PrivateKey(f, pkey, NULL, NULL, 0, NULL, NULL) ? 0 : -4;
    fclose(f);
    /* set file to 0600 if possible (best-effort) */
    chmod(key_path, S_IRUSR | S_IWUSR);

    EVP_PKEY_free(pkey);
    return rc;
}

int certgen_generate_csr_pem(const char *key_path, const char *csr_path, const UsbDeviceInfo *usbInfo) {
    if (!key_path || !csr_path || !usbInfo) return -1;
    ensure_parent_dir(csr_path);

    /* Load private key */
    FILE *kf = fopen(key_path, "rb");
    if (!kf) {
        perror("certgen: fopen key for csr");
        return -2;
    }
    EVP_PKEY *pkey = PEM_read_PrivateKey(kf, NULL, NULL, NULL);
    fclose(kf);
    if (!pkey) {
        fprintf(stderr, "certgen: failed to read private key from %s\n", key_path);
        return -3;
    }

    /* Create X509_REQ */
    X509_REQ *req = X509_REQ_new();
    if (!req) { EVP_PKEY_free(pkey); return -4; }

    if (X509_REQ_set_pubkey(req, pkey) != 1) { X509_REQ_free(req); EVP_PKEY_free(pkey); return -5; }

    /* Compose subject: CN = serial || id || name ; O = name */
    char cn[256]; cn[0]='\0';
    if (usbInfo->serial && usbInfo->serial[0]) sanitize_component(usbInfo->serial, cn, sizeof(cn));
    else if (usbInfo->id && usbInfo->id[0]) sanitize_component(usbInfo->id, cn, sizeof(cn));
    else if (usbInfo->name && usbInfo->name[0]) sanitize_component(usbInfo->name, cn, sizeof(cn));
    else strncpy(cn, "usb-device", sizeof(cn)-1);

    char org[256]; org[0]='\0';
    if (usbInfo->name && usbInfo->name[0]) sanitize_component(usbInfo->name, org, sizeof(org));
    else strncpy(org, "unknown", sizeof(org)-1);

    X509_NAME *name = X509_NAME_new();
    if (!name) { X509_REQ_free(req); EVP_PKEY_free(pkey); return -6; }

    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)cn, -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)org, -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, (unsigned char*)"usb", -1, -1, 0);

    if (X509_REQ_set_subject_name(req, name) != 1) {
        X509_NAME_free(name); X509_REQ_free(req); EVP_PKEY_free(pkey); return -7;
    }
    X509_NAME_free(name);

    /* Optionally add attributes or extensions in the CSR (not required here) */

    /* Sign CSR with private key */
    if (X509_REQ_sign(req, pkey, EVP_sha256()) <= 0) {
        X509_REQ_free(req); EVP_PKEY_free(pkey); return -8;
    }

    /* Write CSR to file */
    FILE *cf = fopen(csr_path, "wb");
    if (!cf) {
        perror("certgen: fopen csr");
        X509_REQ_free(req); EVP_PKEY_free(pkey);
        return -9;
    }
    int rc = PEM_write_X509_REQ(cf, req) ? 0 : -10;
    fclose(cf);

    X509_REQ_free(req);
    EVP_PKEY_free(pkey);
    return rc;
}

// Sign CSR using CA key+cert to produce x509 cert PEM 
int certgen_sign_csr_with_ca(const char *csr_path,
                             const char *ca_cert_path,
                             const char *ca_key_path,
                             const char *out_cert_path,
                             int days) {
    if (!csr_path || !ca_cert_path || !ca_key_path || !out_cert_path) return -1;
    if (days <= 0) days = 365;
    ensure_parent_dir(out_cert_path);

    /* Read CSR */
    FILE *cf = fopen(csr_path, "rb");
    if (!cf) { perror("certgen: fopen csr"); return -2; }
    X509_REQ *req = PEM_read_X509_REQ(cf, NULL, NULL, NULL);
    fclose(cf);
    if (!req) { fprintf(stderr, "certgen: failed to read CSR\n"); return -3; }

    /* Read CA cert */
    FILE *caf = fopen(ca_cert_path, "rb");
    if (!caf) { perror("certgen: fopen ca cert"); X509_REQ_free(req); return -4; }
    X509 *ca = PEM_read_X509(caf, NULL, NULL, NULL);
    fclose(caf);
    if (!ca) { fprintf(stderr, "certgen: failed to read CA cert\n"); X509_REQ_free(req); return -5; }

    /* Read CA private key */
    FILE *kaf = fopen(ca_key_path, "rb");
    if (!kaf) { perror("certgen: fopen ca key"); X509_free(ca); X509_REQ_free(req); return -6; }
    EVP_PKEY *ca_pkey = PEM_read_PrivateKey(kaf, NULL, NULL, NULL);
    fclose(kaf);
    if (!ca_pkey) { fprintf(stderr, "certgen: failed to read CA key\n"); X509_free(ca); X509_REQ_free(req); return -7; }

    /* Create new X509 cert and populate */
    X509 *cert = X509_new();
    if (!cert) { EVP_PKEY_free(ca_pkey); X509_free(ca); X509_REQ_free(req); return -8; }

    /* Version 3 (value 2) */
    X509_set_version(cert, 2);

    /* Serial number - use current time + rand for simplicity */
    ASN1_INTEGER *serial = ASN1_INTEGER_new();
    if (!serial) { X509_free(cert); EVP_PKEY_free(ca_pkey); X509_free(ca); X509_REQ_free(req); return -9; }
    /* create a serial based on time */
    long srl = (long)time(NULL);
    ASN1_INTEGER_set(serial, srl);
    X509_set_serialNumber(cert, serial);
    ASN1_INTEGER_free(serial);

    /* Validity */
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), (long)60*60*24*days);

    /* Set issuer from CA */
    X509_set_issuer_name(cert, X509_get_subject_name(ca));

    /* Set subject from CSR */
    X509_NAME *req_subject = X509_REQ_get_subject_name(req);
    X509_set_subject_name(cert, req_subject);

    /* Set public key from CSR */
    EVP_PKEY *req_pubkey = X509_REQ_get_pubkey(req);
    if (!req_pubkey) { X509_free(cert); EVP_PKEY_free(ca_pkey); X509_free(ca); X509_REQ_free(req); return -10; }
    if (X509_set_pubkey(cert, req_pubkey) != 1) {
        EVP_PKEY_free(req_pubkey); X509_free(cert); EVP_PKEY_free(ca_pkey); X509_free(ca); X509_REQ_free(req); return -11;
    }
    EVP_PKEY_free(req_pubkey);

    /* Optionally copy extensions from CSR (not implemented) */

    /* Add basic extensions: basicConstraints=CA:FALSE, keyUsage, extendedKeyUsage (clientAuth) */
    X509_EXTENSION *ext = NULL;
    X509V3_CTX ctx;
    X509V3_set_ctx(&ctx, ca, cert, NULL, NULL, 0);

    ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_basic_constraints, "CA:FALSE");
    if (ext) { X509_add_ext(cert, ext, -1); X509_EXTENSION_free(ext); }

    ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_key_usage, "digitalSignature,keyEncipherment");
    if (ext) { X509_add_ext(cert, ext, -1); X509_EXTENSION_free(ext); }

    ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_ext_key_usage, "clientAuth");
    if (ext) { X509_add_ext(cert, ext, -1); X509_EXTENSION_free(ext); }

    /* Sign certificate with CA private key */
    if (!X509_sign(cert, ca_pkey, EVP_sha256())) {
        fprintf(stderr, "certgen: failed to sign certificate with CA key\n");
        X509_free(cert); EVP_PKEY_free(ca_pkey); X509_free(ca); X509_REQ_free(req);
        return -12;
    }

    /* Write cert to out_cert_path (PEM) */
    FILE *of = fopen(out_cert_path, "wb");
    if (!of) {
        perror("certgen: fopen out cert");
        X509_free(cert); EVP_PKEY_free(ca_pkey); X509_free(ca); X509_REQ_free(req);
        return -13;
    }
    int rc = PEM_write_X509(of, cert) ? 0 : -14;
    fclose(of);

    /* cleanup */
    X509_free(cert);
    EVP_PKEY_free(ca_pkey);
    X509_free(ca);
    X509_REQ_free(req);

    return rc;
}
