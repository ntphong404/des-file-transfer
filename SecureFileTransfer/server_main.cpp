/*
 * Author: PhongPKF
 * File: server_main.cpp
 * Description: Server application - listens for incoming connections, receives
 *              encrypted data from client, decrypts using DES, and saves plaintext file.
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
    std::cout << "Usage: " << programName << " <port> <output_file> <key>" << std::endl;
    std::cout << "Example: " << programName << " 5000 data/decrypted.txt 12345678" << std::endl;
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
    if (argc != 4)
    {
        printUsage(argv[0]);
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string outputFile = argv[2];
    std::string keyStr = argv[3];

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

    std::cout << "=== DES File Reception & Decryption (Server) ===" << std::endl;
    std::cout << "Listening on port: " << port << std::endl;
    std::cout << "Output file: " << outputFile << std::endl;
    std::cout << "Key: " << keyStr << std::endl;

    // Initialize socket library
    if (!initializeSocketLibrary())
    {
        std::cerr << "Failed to initialize socket library" << std::endl;
        return 1;
    }

    // Create server socket
    std::cout << "\n[1] Creating server socket..." << std::endl;
    SocketHandle serverSocket = createServerSocket(port);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create server socket" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }

    // Accept client connection
    std::cout << "\n[2] Waiting for client connection..." << std::endl;
    SocketHandle clientSocket = acceptConnection(serverSocket);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to accept client connection" << std::endl;
        closeSocket(serverSocket);
        cleanupSocketLibrary();
        return 1;
    }

    // Close server socket (we don't need it anymore)
    closeSocket(serverSocket);

    // Receive file size
    std::cout << "\n[3] Receiving file size..." << std::endl;
    std::vector<uint8_t> sizeBuffer;
    int bytesRecv = receiveExact(clientSocket, sizeBuffer, 8);
    if (bytesRecv != 8)
    {
        std::cerr << "Failed to receive file size (got " << bytesRecv << " bytes)" << std::endl;
        closeSocket(clientSocket);
        cleanupSocketLibrary();
        return 1;
    }

    uint64_t fileSize = 0;
    for (int i = 0; i < 8; i++)
    {
        fileSize = (fileSize << 8) | sizeBuffer[i];
    }
    std::cout << "    File size: " << fileSize << " bytes" << std::endl;

    // Receive encrypted data
    std::cout << "\n[4] Receiving encrypted data..." << std::endl;
    std::vector<uint8_t> ciphertext;
    int recvResult = receiveExact(clientSocket, ciphertext, fileSize);
    if (recvResult != (int)fileSize)
    {
        std::cerr << "Failed to receive all encrypted data (expected " << fileSize
                  << " bytes, got " << recvResult << " bytes)" << std::endl;
        closeSocket(clientSocket);
        cleanupSocketLibrary();
        return 1;
    }
    std::cout << "    Received: " << fileSize << " bytes" << std::endl;

    // Close client socket
    closeSocket(clientSocket);

    // Verify ciphertext size
    if (ciphertext.size() % BLOCK_SIZE != 0)
    {
        std::cerr << "Error: Ciphertext size is not multiple of block size" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }

    // Generate round keys
    std::cout << "\n[5] Generating DES round keys..." << std::endl;
    RoundKeys roundKeys;
    generateRoundKeys(desKey, roundKeys);
    std::cout << "    Round keys generated (16 keys)" << std::endl;

    // Decrypt data
    std::cout << "\n[6] Decrypting data (ECB mode)..." << std::endl;
    std::vector<uint8_t> decrypted;
    for (size_t i = 0; i < ciphertext.size(); i += BLOCK_SIZE)
    {
        uint8_t decryptedBlock[BLOCK_SIZE];
        decryptBlock(ciphertext.data() + i, decryptedBlock, roundKeys);
        decrypted.insert(decrypted.end(), decryptedBlock, decryptedBlock + BLOCK_SIZE);
    }
    std::cout << "    Decrypted size (before unpadding): " << decrypted.size() << " bytes" << std::endl;

    // Remove padding
    std::cout << "\n[7] Removing PKCS#7 padding..." << std::endl;
    if (!unpadData(decrypted))
    {
        std::cerr << "Error: Invalid PKCS#7 padding - decryption may have failed" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }
    std::cout << "    Plaintext size (after unpadding): " << decrypted.size() << " bytes" << std::endl;

    // Save plaintext to file
    std::cout << "\n[8] Saving decrypted data to file..." << std::endl;
    if (!writeFile(outputFile, decrypted))
    {
        std::cerr << "Failed to write output file" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }
    std::cout << "    File saved successfully!" << std::endl;

    // Cleanup
    std::cout << "\n[9] Cleaning up..." << std::endl;
    cleanupSocketLibrary();

    std::cout << "\n=== Reception & Decryption Complete ===" << std::endl;
    return 0;
}
