#include "crypto.h"
#include "log.h"

//
// This file contains STUBS for a cryptographic library.
// A real implementation would require a dedicated, well-tested crypto library.
// For the purpose of the OS framework, these functions simulate the API.
//

void sha256_hash(const uint8_t* data, size_t len, uint8_t* hash_out) {
    // STUB: This is not a real hash function.
    // It just provides a predictable "hash" for demonstration.
    klog(LOG_INFO, "CRYPTO_STUB: sha256_hash called.");
    for (int i = 0; i < SHA256_HASH_SIZE; i++) {
        hash_out[i] = (uint8_t)(i ^ (len & 0xFF));
    }
}

int rsa_verify_signature(const uint8_t* signature, const uint8_t* hash) {
    // STUB: This is not real signature verification.
    // It always returns true to simulate a valid signature.
    klog(LOG_INFO, "CRYPTO_STUB: rsa_verify_signature called. Returning TRUE.");
    (void)signature; // Unused in stub
    (void)hash;      // Unused in stub
    return 1; // 1 for success (signature is valid)
}
