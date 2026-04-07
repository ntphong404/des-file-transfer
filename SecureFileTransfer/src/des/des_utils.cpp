/*
 * Author: PhongPKF
 * File: des_utils.cpp
 * Description: Implements utility functions for file I/O, PKCS#7 padding,
 *              and file-level encryption/decryption using DES.
 */

#include "../../include/des/des_utils.h"
#include "../../include/des/des_core.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <windows.h>

namespace DES
{

    // Chuyển UTF-8 string → wstring để Windows file API hiểu tên tiếng Việt
    static std::wstring utf8ToWide(const std::string &s)
    {
        if (s.empty())
            return {};
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring ws(len - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
        return ws;
    }

    // Apply PKCS#7 padding
    std::vector<uint8_t> padData(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> padded = data;
        int paddingLen = BLOCK_SIZE - (data.size() % BLOCK_SIZE);
        if (paddingLen == 0)
            paddingLen = BLOCK_SIZE;

        for (int i = 0; i < paddingLen; i++)
        {
            padded.push_back(static_cast<uint8_t>(paddingLen));
        }
        return padded;
    }

    // Remove PKCS#7 padding
    bool unpadData(std::vector<uint8_t> &data)
    {
        if (data.empty())
        {
            std::cerr << "Error: Data is empty" << std::endl;
            return false;
        }

        uint8_t paddingLen = data.back();

        if (paddingLen > BLOCK_SIZE || paddingLen == 0)
        {
            std::cerr << "Error: Invalid padding length: " << (int)paddingLen
                      << " (expected 1-" << BLOCK_SIZE << ")" << std::endl;
            return false;
        }

        if (data.size() < paddingLen)
        {
            std::cerr << "Error: Data size (" << data.size()
                      << ") smaller than padding length (" << (int)paddingLen << ")" << std::endl;
            return false;
        }

        // Verify all padding bytes
        for (uint8_t i = 1; i <= paddingLen; i++)
        {
            if (data[data.size() - i] != paddingLen)
            {
                std::cerr << "Error: Padding byte " << (int)i << " is 0x" << std::hex
                          << (int)data[data.size() - i] << ", expected 0x" << (int)paddingLen
                          << std::dec << std::endl;
                return false;
            }
        }

        data.erase(data.end() - paddingLen, data.end());
        return true;
    }

    // Read file into memory (hỗ trợ tên file UTF-8 / tiếng Việt)
    std::vector<uint8_t> readFile(const std::string &filename)
    {
        FILE *fp = _wfopen(utf8ToWide(filename).c_str(), L"rb");
        if (!fp)
        {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return {};
        }
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<uint8_t> data(sz);
        fread(data.data(), 1, sz, fp);
        fclose(fp);
        return data;
    }

    // Write byte vector to file (hỗ trợ tên file UTF-8 / tiếng Việt)
    bool writeFile(const std::string &filename, const std::vector<uint8_t> &data)
    {
        FILE *fp = _wfopen(utf8ToWide(filename).c_str(), L"wb");
        if (!fp)
        {
            std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
            return false;
        }
        fwrite(data.data(), 1, data.size(), fp);
        fclose(fp);
        return true;
    }

    // Convert byte to binary string
    std::string byteToBitString(uint8_t byte)
    {
        std::string result;
        for (int i = 7; i >= 0; i--)
        {
            result += ((byte >> i) & 1) ? '1' : '0';
        }
        return result;
    }

    // Convert nibble to hex character
    char nibbleToHexChar(uint8_t nibble)
    {
        if (nibble < 10)
            return '0' + nibble;
        return 'A' + (nibble - 10);
    }

    // Encrypt entire file
    bool encryptFile(const std::string &inputFile, const std::string &outputFile,
                     const uint8_t *desKey)
    {
        // Read plaintext file
        std::vector<uint8_t> plaintext = readFile(inputFile);
        if (plaintext.empty() && inputFile != "/dev/null")
        {
            std::cerr << "Error: Cannot read input file" << std::endl;
            return false;
        }

        // Apply padding
        std::vector<uint8_t> padded = padData(plaintext);

        // Generate round keys
        RoundKeys roundKeys;
        generateRoundKeys(desKey, roundKeys);

        // Encrypt each block
        std::vector<uint8_t> ciphertext;
        for (size_t i = 0; i < padded.size(); i += BLOCK_SIZE)
        {
            uint8_t encryptedBlock[BLOCK_SIZE];
            encryptBlock(padded.data() + i, encryptedBlock, roundKeys);
            ciphertext.insert(ciphertext.end(), encryptedBlock, encryptedBlock + BLOCK_SIZE);
        }

        // Write ciphertext to file
        return writeFile(outputFile, ciphertext);
    }

    // Decrypt entire file
    bool decryptFile(const std::string &inputFile, const std::string &outputFile,
                     const uint8_t *desKey)
    {
        // Read ciphertext file
        std::vector<uint8_t> ciphertext = readFile(inputFile);
        if (ciphertext.empty())
        {
            std::cerr << "Error: Cannot read input file" << std::endl;
            return false;
        }

        // Verify file size is multiple of block size
        if (ciphertext.size() % BLOCK_SIZE != 0)
        {
            std::cerr << "Error: Ciphertext size is not multiple of block size" << std::endl;
            return false;
        }

        // Generate round keys
        RoundKeys roundKeys;
        generateRoundKeys(desKey, roundKeys);

        // Decrypt each block
        std::vector<uint8_t> decrypted;
        for (size_t i = 0; i < ciphertext.size(); i += BLOCK_SIZE)
        {
            uint8_t decryptedBlock[BLOCK_SIZE];
            decryptBlock(ciphertext.data() + i, decryptedBlock, roundKeys);
            decrypted.insert(decrypted.end(), decryptedBlock, decryptedBlock + BLOCK_SIZE);
        }

        // Remove padding
        if (!unpadData(decrypted))
        {
            std::cerr << "Error: Invalid PKCS#7 padding" << std::endl;
            return false;
        }

        // Write plaintext to file
        return writeFile(outputFile, decrypted);
    }

} // namespace DES
