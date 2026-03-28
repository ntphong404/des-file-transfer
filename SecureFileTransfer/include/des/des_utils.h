/*
 * Author: PhongPKF
 * File: des_utils.h
 * Description: Declares utility functions for DES including file I/O,
 *              PKCS#7 padding, and bit manipulation helpers.
 */

#ifndef DES_UTILS_H
#define DES_UTILS_H

#include <cstdint>
#include <vector>
#include <string>

namespace DES
{

    /*
     * Apply PKCS#7 padding to data
     * If data length is N and block size is 8, pad with (8 - N%8) bytes
     * each with value (8 - N%8). If N%8 == 0, add a full block of padding.
     * Input:  data (byte vector)
     * Output: padded data (length will be multiple of 8)
     */
    std::vector<uint8_t> padData(const std::vector<uint8_t> &data);

    /*
     * Remove PKCS#7 padding from data
     * Input:  padded data
     * Output: unpadded data
     * Returns: true if padding is valid, false otherwise
     */
    bool unpadData(std::vector<uint8_t> &data);

    /*
     * Read entire file into memory as byte vector
     * Input:  filename
     * Output: file contents as vector of bytes
     */
    std::vector<uint8_t> readFile(const std::string &filename);

    /*
     * Write byte vector to file
     * Input:  filename, data vector
     * Output: true if successful, false otherwise
     */
    bool writeFile(const std::string &filename, const std::vector<uint8_t> &data);

    /*
     * Convert 8-bit byte to 8-bit string (for bit manipulation)
     * Input:  byte value
     * Output: 8-bit binary string representation
     */
    std::string byteToBitString(uint8_t byte);

    /*
     * Convert 4-bit nibble to 2-character hex string
     * Input:  value (0-15)
     * Output: hex string (e.g., "A", "F")
     */
    char nibbleToHexChar(uint8_t nibble);

    /*
     * Encrypt entire file using DES (ECB mode with PKCS#7 padding)
     * Input:  input filename, output filename, 8-byte DES key
     * Output: true if successful
     */
    bool encryptFile(const std::string &inputFile, const std::string &outputFile,
                     const uint8_t *desKey);

    /*
     * Decrypt entire file using DES (ECB mode with PKCS#7 padding)
     * Input:  input filename, output filename, 8-byte DES key
     * Output: true if successful
     */
    bool decryptFile(const std::string &inputFile, const std::string &outputFile,
                     const uint8_t *desKey);

} // namespace DES

#endif // DES_UTILS_H
