/*
 * Author: PhongPKF
 * File: des_tables.h
 * Description: Declares DES cryptographic transformation tables and constants
 *              including Initial Permutation, S-boxes, P-box, PC-1, PC-2.
 */

#ifndef DES_TABLES_H
#define DES_TABLES_H

#include <cstdint>

namespace DES
{

    // Initial Permutation table (64-bit)
    extern const int IP[64];

    // Final Permutation table (inverse of IP)
    extern const int IP_INV[64];

    // Expansion function (32-bit to 48-bit)
    extern const int E[48];

    // S-box substitution tables (8 S-boxes, each 4x16)
    extern const int S_BOX[8][4][16];

    // P-box permutation (32-bit)
    extern const int P[32];

    // Permuted Choice 1 (56 bits from 64-bit key)
    extern const int PC1[56];

    // Permuted Choice 2 (48 bits from 56-bit subkey)
    extern const int PC2[48];

    // Left shifts for key schedule
    extern const int LEFT_SHIFTS[16];

} // namespace DES

#endif // DES_TABLES_H
