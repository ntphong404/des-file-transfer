/*
 * Author: PhongPKF
 * File: des_core.h
 * Description: Declares core DES encryption/decryption functions including
 *              16-round Feistel network, key scheduling, and block cipher operations.
 */

#ifndef DES_CORE_H
#define DES_CORE_H

#include <cstdint>
#include <array>
#include <vector>

namespace DES
{

    // DES block size: 64 bits = 8 bytes
    static constexpr int BLOCK_SIZE = 8;

    // DES key size: 56 bits effective (64 bits with parity)
    static constexpr int KEY_SIZE = 8;

    // Number of DES rounds
    static constexpr int NUM_ROUNDS = 16;

    // Type for DES subkeys (48 bits each)
    using SubKey = std::array<uint8_t, 6>; // 6 bytes = 48 bits

    // Type for round keys
    using RoundKeys = std::array<SubKey, NUM_ROUNDS>;

    /*
     * Generate round keys from a 64-bit master key
     * Input:  key (8 bytes)
     * Output: roundKeys (16 subkeys of 6 bytes each)
     */
    void generateRoundKeys(const uint8_t *key, RoundKeys &roundKeys);

    /*
     * DES F-function: Core cryptographic transformation in one round
     * Input:  right half (4 bytes), round key (6 bytes)
     * Output: result (4 bytes)
     */
    uint32_t desF(uint32_t right, const SubKey &roundKey);

    /*
     * Encrypt a single 64-bit DES block
     * Input:  plaintext block (8 bytes), round keys
     * Output: ciphertext block (8 bytes)
     */
    void encryptBlock(const uint8_t *plaintext, uint8_t *ciphertext, const RoundKeys &roundKeys);

    /*
     * Decrypt a single 64-bit DES block
     * Input:  ciphertext block (8 bytes), round keys
     * Output: plaintext block (8 bytes)
     */
    void decryptBlock(const uint8_t *ciphertext, uint8_t *plaintext, const RoundKeys &roundKeys);

} // namespace DES

#endif // DES_CORE_H
