/*
 * Author: PhongPKF
 * File: client_main.cpp
 * Description: Client application - reads plaintext file, encrypts using DES,
 *              and sends encrypted data to remote server via TCP socket.
 */

#include "include/des/des_core.h"
#include "include/des/des_utils.h"
#include "include/network/socket_utils.h"
#include <iostream>
#include <cstring>
#include <fstream>

using namespace DES;
using namespace Network;

// Helper function to print usage
void printUsage(const char *programName)
{
    std::cout << "Usage: " << programName << " <server_ip> <port> <input_file> <key>" << std::endl;
    std::cout << "Example: " << programName << " 127.0.0.1 5000 data/plain.txt 12345678" << std::endl;
}

// Helper function to parse 8-character string to 8-byte key
bool parseKey(const std::string &keyStr, uint8_t *key)
{
    if (keyStr.length() != 8)
    {
        std::cerr << "Error: Key must be exactly 8 characters long" << std::endl;
        return false;
    }
    std::memcpy(key, keyStr.c_str(), 8);
    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string serverIP = argv[1];
    int port = std::atoi(argv[2]);
    std::string inputFile = argv[3];
    std::string keyStr = argv[4];

    // Validate input
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
        return 1;
    }

    // Parse DES key
    uint8_t desKey[KEY_SIZE];
    if (!parseKey(keyStr, desKey))
    {
        return 1;
    }

    std::cout << "=== DES File Encryption & Transmission (Client) ===" << std::endl;
    std::cout << "Server: " << serverIP << ":" << port << std::endl;
    std::cout << "Input file: " << inputFile << std::endl;
    std::cout << "Key: " << keyStr << std::endl;

    // Initialize socket library
    if (!initializeSocketLibrary())
    {
        std::cerr << "Failed to initialize socket library" << std::endl;
        return 1;
    }

    // Read input file
    std::cout << "\n[1] Reading input file..." << std::endl;
    std::vector<uint8_t> plaintext = readFile(inputFile);
    if (plaintext.empty())
    {
        std::cerr << "Error: Cannot read input file or file is empty" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }
    std::cout << "    Plaintext size: " << plaintext.size() << " bytes" << std::endl;

    // Apply padding
    std::cout << "\n[2] Applying PKCS#7 padding..." << std::endl;
    std::vector<uint8_t> padded = padData(plaintext);
    std::cout << "    Padded size: " << padded.size() << " bytes" << std::endl;

    // Generate round keys
    std::cout << "\n[3] Generating DES round keys..." << std::endl;
    RoundKeys roundKeys;
    generateRoundKeys(desKey, roundKeys);
    std::cout << "    Round keys generated (16 keys)" << std::endl;

    // Encrypt file
    std::cout << "\n[4] Encrypting file (ECB mode)..." << std::endl;
    std::vector<uint8_t> ciphertext;
    for (size_t i = 0; i < padded.size(); i += BLOCK_SIZE)
    {
        uint8_t encryptedBlock[BLOCK_SIZE];
        encryptBlock(padded.data() + i, encryptedBlock, roundKeys);
        ciphertext.insert(ciphertext.end(), encryptedBlock, encryptedBlock + BLOCK_SIZE);
    }
    std::cout << "    Ciphertext size: " << ciphertext.size() << " bytes" << std::endl;
    std::cout << "    Number of blocks: " << (ciphertext.size() / BLOCK_SIZE) << std::endl;

    // Connect to server
    std::cout << "\n[5] Connecting to server..." << std::endl;
    SocketHandle serverSocket = connectToServer(serverIP, port);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to connect to server" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }

    // Send file size (8 bytes)
    std::cout << "\n[6] Sending encrypted data..." << std::endl;
    uint64_t fileSize = ciphertext.size();
    uint8_t sizeBuffer[8];
    for (int i = 7; i >= 0; i--)
    {
        sizeBuffer[i] = (uint8_t)(fileSize & 0xFF);
        fileSize >>= 8;
    }

    if (!sendData(serverSocket, sizeBuffer, 8))
    {
        std::cerr << "Failed to send file size" << std::endl;
        closeSocket(serverSocket);
        cleanupSocketLibrary();
        return 1;
    }

    // Send encrypted data
    if (!sendData(serverSocket, ciphertext.data(), ciphertext.size()))
    {
        std::cerr << "Failed to send encrypted data" << std::endl;
        closeSocket(serverSocket);
        cleanupSocketLibrary();
        return 1;
    }

    std::cout << "    Data sent successfully!" << std::endl;

    // Cleanup
    std::cout << "\n[7] Cleaning up..." << std::endl;
    closeSocket(serverSocket);
    cleanupSocketLibrary();

    std::cout << "\n=== Transmission Complete ===" << std::endl;
    return 0;
}
