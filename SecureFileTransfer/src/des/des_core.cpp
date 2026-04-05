/*
 * Author: PhongPKF
 * File: des_core.cpp
 * Description: Implements core DES encryption/decryption including 16-round Feistel network,
 *              key schedule generation, and block-level cipher operations.
 */

#include "../../include/des/des_core.h"
#include "../../include/des/des_tables.h"
#include <cstring>
#include <algorithm>

namespace DES
{

    // Helper: Convert 8 bytes to 64-bit value
    static uint64_t bytesToUint64(const uint8_t *bytes)
    {
        uint64_t result = 0;
        for (int i = 0; i < 8; i++)
        {
            result = (result << 8) | bytes[i];
        }
        return result;
    }

    // Helper: Convert 64-bit value to 8 bytes
    static void uint64ToBytes(uint64_t value, uint8_t *bytes)
    {
        for (int i = 7; i >= 0; i--)
        {
            bytes[i] = static_cast<uint8_t>(value & 0xFF);
            value >>= 8;
        }
    }

    // Helper: Extract bit at position (1-indexed)
    static int getBit(const uint8_t *data, int position)
    {
        position--; // Convert to 0-indexed
        int byteIndex = position / 8;
        int bitIndex = 7 - (position % 8);
        return (data[byteIndex] >> bitIndex) & 1;
    }

    // Helper: Get multiple bits and form a value
    static uint32_t getBits(const uint8_t *data, const int *positions, int count)
    {
        uint32_t result = 0;
        for (int i = 0; i < count; i++)
        {
            result = (result << 1) | getBit(data, positions[i]);
        }
        return result;
    }

    // Helper: Perform bit permutation
    static void permute(const uint8_t *input, uint8_t *output, const int *table, int inputBits, int outputBits)
    {
        std::fill(output, output + (outputBits + 7) / 8, 0);
        for (int i = 0; i < outputBits; i++)
        {
            int bitValue = getBit(input, table[i]);
            int outputByteIndex = i / 8;
            int outputBitIndex = 7 - (i % 8);
            output[outputByteIndex] |= (bitValue << outputBitIndex);
        }
    }

    // Generate round keys from master key
    void generateRoundKeys(const uint8_t *key, RoundKeys &roundKeys)
    {
        uint8_t pc1Output[7]; // 56 bits = 7 bytes

        // Apply PC1 to key (64 -> 56 bits)
        permute(key, pc1Output, (int *)PC1, 64, 56);

        // Split into C0 and D0 (28 bits each)
        uint32_t c = 0, d = 0;
        for (int i = 0; i < 28; i++)
        {
            c = (c << 1) | getBit(pc1Output, i + 1);
            d = (d << 1) | getBit(pc1Output, i + 29);
        }

        // Generate 16 round keys
        for (int round = 0; round < NUM_ROUNDS; round++)
        {
            // Left rotate C and D
            int shifts = LEFT_SHIFTS[round];
            c = ((c << shifts) | (c >> (28 - shifts))) & 0x0FFFFFFF;
            d = ((d << shifts) | (d >> (28 - shifts))) & 0x0FFFFFFF;

            // Combine C and D into 56-bit value
            uint8_t cd[7];
            std::fill(cd, cd + 7, 0);
            for (int i = 0; i < 28; i++)
            {
                int bitC = (c >> (27 - i)) & 1;
                int bitD = (d >> (27 - i)) & 1;
                int byteIdx = i / 8;
                int bitIdx = 7 - (i % 8);
                cd[byteIdx] |= (bitC << bitIdx);

                byteIdx = (28 + i) / 8;
                bitIdx = 7 - ((28 + i) % 8);
                cd[byteIdx] |= (bitD << bitIdx);
            }

            // Apply PC2 to get 48-bit subkey
            permute(cd, (uint8_t *)&roundKeys[round], (int *)PC2, 56, 48);
        }
    }

    // DES F-function
    uint32_t desF(uint32_t right, const SubKey &roundKey)
    {
        // Step 1: Expand 32-bit to 48-bit
        uint8_t expanded[6];
        std::fill(expanded, expanded + 6, 0);
        for (int i = 0; i < 48; i++)
        {
            int position = E[i];
            int bitValue = (right >> (32 - position)) & 1;
            int byteIdx = i / 8;
            int bitIdx = 7 - (i % 8);
            expanded[byteIdx] |= (bitValue << bitIdx);
        }

        // Step 2: XOR with round key
        for (int i = 0; i < 6; i++)
        {
            expanded[i] ^= roundKey[i];
        }

        // Step 3: S-box substitution
        uint8_t sboxOutput[4];
        std::fill(sboxOutput, sboxOutput + 4, 0);
        for (int i = 0; i < 8; i++)
        {
            // Extract 6 bits for S-box
            int row = ((expanded[i * 6 / 8] >> (7 - ((i * 6) % 8))) & 1) << 1;
            row |= (expanded[(i * 6 + 4) / 8] >> (7 - ((i * 6 + 4) % 8))) & 1;

            int col = 0;
            for (int j = 1; j < 5; j++)
            {
                col = (col << 1) | ((expanded[(i * 6 + j) / 8] >> (7 - ((i * 6 + j) % 8))) & 1);
            }

            int sboxValue = S_BOX[i][row][col];
            int outputByteIdx = i / 2;
            int outputBitIdx = (i % 2) ? 0 : 4;
            sboxOutput[outputByteIdx] |= (sboxValue << outputBitIdx);
        }

        // Step 4: P-box permutation
        uint8_t pboxOutput[4];
        std::fill(pboxOutput, pboxOutput + 4, 0);
        for (int i = 0; i < 32; i++)
        {
            int position = P[i];
            int bitValue = (sboxOutput[(position - 1) / 8] >> (7 - ((position - 1) % 8))) & 1;
            int byteIdx = i / 8;
            int bitIdx = 7 - (i % 8);
            pboxOutput[byteIdx] |= (bitValue << bitIdx);
        }

        return ((uint32_t)pboxOutput[0] << 24) | ((uint32_t)pboxOutput[1] << 16) |
               ((uint32_t)pboxOutput[2] << 8) | pboxOutput[3];
    }

    // Encrypt single block
    void encryptBlock(const uint8_t *plaintext, uint8_t *ciphertext, const RoundKeys &roundKeys)
    {
        uint8_t permuted[8];

        // Apply initial permutation
        permute(plaintext, permuted, (int *)IP, 64, 64);

        // Split into left and right halves
        uint32_t left = ((uint32_t)permuted[0] << 24) | ((uint32_t)permuted[1] << 16) |
                        ((uint32_t)permuted[2] << 8) | permuted[3];
        uint32_t right = ((uint32_t)permuted[4] << 24) | ((uint32_t)permuted[5] << 16) |
                         ((uint32_t)permuted[6] << 8) | permuted[7];

        // 16 rounds of Feistel network
        for (int round = 0; round < NUM_ROUNDS; round++)
        {
            uint32_t temp = right;
            right = left ^ desF(right, roundKeys[round]);
            left = temp;
        }

        // Combine right and left (note the order!)
        uint8_t combined[8];
        combined[0] = (right >> 24) & 0xFF;
        combined[1] = (right >> 16) & 0xFF;
        combined[2] = (right >> 8) & 0xFF;
        combined[3] = right & 0xFF;
        combined[4] = (left >> 24) & 0xFF;
        combined[5] = (left >> 16) & 0xFF;
        combined[6] = (left >> 8) & 0xFF;
        combined[7] = left & 0xFF;

        // Apply final permutation
        permute(combined, ciphertext, (int *)IP_INV, 64, 64);
    }

    // Decrypt single block
    void decryptBlock(const uint8_t *ciphertext, uint8_t *plaintext, const RoundKeys &roundKeys)
    {
        uint8_t permuted[8];

        // Apply initial permutation
        permute(ciphertext, permuted, (int *)IP, 64, 64);

        // Split into left and right halves
        uint32_t left = ((uint32_t)permuted[0] << 24) | ((uint32_t)permuted[1] << 16) |
                        ((uint32_t)permuted[2] << 8) | permuted[3];
        uint32_t right = ((uint32_t)permuted[4] << 24) | ((uint32_t)permuted[5] << 16) |
                         ((uint32_t)permuted[6] << 8) | permuted[7];

        // 16 rounds of Feistel network (in reverse order)
        for (int round = NUM_ROUNDS - 1; round >= 0; round--)
        {
            uint32_t temp = left;
            left = right;
            right = temp ^ desF(right, roundKeys[round]);
        }

        // Combine right and left (note the order!)
        uint8_t combined[8];
        combined[0] = (right >> 24) & 0xFF;
        combined[1] = (right >> 16) & 0xFF;
        combined[2] = (right >> 8) & 0xFF;
        combined[3] = right & 0xFF;
        combined[4] = (left >> 24) & 0xFF;
        combined[5] = (left >> 16) & 0xFF;
        combined[6] = (left >> 8) & 0xFF;
        combined[7] = left & 0xFF;

        // Apply final permutation
        permute(combined, plaintext, (int *)IP_INV, 64, 64);
    }

    // 3DES Encrypt
    void encryptBlock3DES(const uint8_t *plaintext, uint8_t *ciphertext, 
                          const RoundKeys &rk1, const RoundKeys &rk2, const RoundKeys &rk3)
    {
        uint8_t temp1[8], temp2[8];
        encryptBlock(plaintext, temp1, rk1);
        decryptBlock(temp1, temp2, rk2);
        encryptBlock(temp2, ciphertext, rk3);
    }

    // 3DES Decrypt
    void decryptBlock3DES(const uint8_t *ciphertext, uint8_t *plaintext, 
                          const RoundKeys &rk1, const RoundKeys &rk2, const RoundKeys &rk3)
    {
        uint8_t temp1[8], temp2[8];
        decryptBlock(ciphertext, temp1, rk3);
        encryptBlock(temp1, temp2, rk2);
        decryptBlock(temp2, plaintext, rk1);
    }

} // namespace DES
