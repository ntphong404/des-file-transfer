/*
 * Author: PhongPKF
 * File: client_main.cpp
 * Description: Client - mã hóa và gửi NHIỀU file kèm tên file qua TCP.
 *
 * Protocol (mới):
 *   [4B: num_files]
 *   For each file:
 *     [4B: filename_len] [filename_len bytes: filename]
 *     [8B: encrypted_data_size] [encrypted_data bytes]
 *
 * Usage: client_send.exe <server_ip> <port> <key> <file1> [file2] [file3] ...
 */

#include "include/des/des_core.h"
#include "include/des/des_utils.h"
#include "include/network/socket_utils.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <string>

using namespace DES;
using namespace Network;

void printUsage(const char *programName)
{
    std::cout << "Usage: " << programName
              << " <server_ip> <port> <key> <file1> [file2] ..." << std::endl;
    std::cout << "Example: " << programName
              << " 127.0.0.1 5000 12345678 plain.txt photo.jpg" << std::endl;
}

// Parse key: require exactly 8 bytes (use first 8 chars)
bool parseKey(const std::string &keyStr, uint8_t *key)
{
    if (keyStr.length() < 8)
    {
        std::cerr << "Error: Key must be at least 8 characters" << std::endl;
        return false;
    }
    std::memcpy(key, keyStr.c_str(), KEY_SIZE);
    return true;
}

// Send uint32_t big-endian (4 bytes)
bool sendUint32(SocketHandle sock, uint32_t value)
{
    uint8_t buf[4];
    buf[0] = (value >> 24) & 0xFF;
    buf[1] = (value >> 16) & 0xFF;
    buf[2] = (value >> 8) & 0xFF;
    buf[3] =  value        & 0xFF;
    return sendData(sock, buf, 4);
}

// Send uint64_t big-endian (8 bytes)
bool sendUint64(SocketHandle sock, uint64_t value)
{
    uint8_t buf[8];
    for (int i = 7; i >= 0; i--)
    {
        buf[i] = value & 0xFF;
        value >>= 8;
    }
    return sendData(sock, buf, 8);
}

// Extract just the filename (basename) from a full path
std::string getBasename(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

int main(int argc, char *argv[])
{
    // Need at least: exe ip port key file1
    if (argc < 5)
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string serverIP = argv[1];
    int port = std::atoi(argv[2]);
    std::string keyStr = argv[3];

    // Collect all input file paths
    std::vector<std::string> inputFiles;
    for (int i = 4; i < argc; i++)
        inputFiles.push_back(argv[i]);

    // Validate
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
        return 1;
    }

    uint8_t desKey[KEY_SIZE];
    if (!parseKey(keyStr, desKey))
        return 1;

    std::cout << "=== DES Multi-File Encryption & Transmission (Client) ===" << std::endl;
    std::cout << "Server : " << serverIP << ":" << port << std::endl;
    std::cout << "Files  : " << inputFiles.size() << std::endl;

    // Init socket library
    if (!initializeSocketLibrary())
    {
        std::cerr << "Failed to initialize socket library" << std::endl;
        return 1;
    }

    // ----- Pre-encrypt all files in memory -----
    struct FileData {
        std::string filename;
        std::vector<uint8_t> ciphertext;
    };

    RoundKeys roundKeys;
    generateRoundKeys(desKey, roundKeys);

    std::vector<FileData> filesData;
    int fileIndex = 0;
    for (const auto &path : inputFiles)
    {
        fileIndex++;
        std::string filename = getBasename(path);
        std::cout << "\n[" << fileIndex << "/" << inputFiles.size()
                  << "] Reading & encrypting: " << filename << std::endl;

        std::vector<uint8_t> plaintext = readFile(path);
        if (plaintext.empty())
        {
            std::cerr << "  WARNING: Cannot read '" << path << "', skipping." << std::endl;
            continue;
        }
        std::cout << "  Plaintext : " << plaintext.size() << " bytes" << std::endl;

        std::vector<uint8_t> padded = padData(plaintext);

        std::vector<uint8_t> ciphertext;
        ciphertext.reserve(padded.size());
        for (size_t i = 0; i < padded.size(); i += BLOCK_SIZE)
        {
            uint8_t encBlock[BLOCK_SIZE];
            encryptBlock(padded.data() + i, encBlock, roundKeys);
            ciphertext.insert(ciphertext.end(), encBlock, encBlock + BLOCK_SIZE);
        }
        std::cout << "  Ciphertext: " << ciphertext.size() << " bytes" << std::endl;

        filesData.push_back({filename, std::move(ciphertext)});
    }

    if (filesData.empty())
    {
        std::cerr << "No valid files to send." << std::endl;
        cleanupSocketLibrary();
        return 1;
    }

    // ----- Connect -----
    std::cout << "\n[Connecting] " << serverIP << ":" << port << " ..." << std::endl;
    SocketHandle sock = connectToServer(serverIP, (uint16_t)port);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Failed to connect to server" << std::endl;
        cleanupSocketLibrary();
        return 1;
    }

    // ----- Send number of files -----
    uint32_t numFiles = (uint32_t)filesData.size();
    if (!sendUint32(sock, numFiles))
    {
        std::cerr << "Failed to send file count" << std::endl;
        closeSocket(sock);
        cleanupSocketLibrary();
        return 1;
    }
    std::cout << "[Sending] " << numFiles << " file(s) to server..." << std::endl;

    // ----- Send each file -----
    for (uint32_t i = 0; i < numFiles; i++)
    {
        const FileData &fd = filesData[i];
        std::cout << "\n  [" << (i + 1) << "/" << numFiles << "] " << fd.filename << std::endl;

        // 1) filename length (4 bytes)
        uint32_t nameLen = (uint32_t)fd.filename.size();
        if (!sendUint32(sock, nameLen))
        {
            std::cerr << "  Failed to send filename length" << std::endl;
            closeSocket(sock);
            cleanupSocketLibrary();
            return 1;
        }

        // 2) filename bytes
        if (!sendData(sock, (const uint8_t *)fd.filename.data(), nameLen))
        {
            std::cerr << "  Failed to send filename" << std::endl;
            closeSocket(sock);
            cleanupSocketLibrary();
            return 1;
        }

        // 3) encrypted data size (8 bytes)
        uint64_t dataSize = (uint64_t)fd.ciphertext.size();
        if (!sendUint64(sock, dataSize))
        {
            std::cerr << "  Failed to send data size" << std::endl;
            closeSocket(sock);
            cleanupSocketLibrary();
            return 1;
        }

        // 4) encrypted data
        if (!sendData(sock, fd.ciphertext.data(), fd.ciphertext.size()))
        {
            std::cerr << "  Failed to send data" << std::endl;
            closeSocket(sock);
            cleanupSocketLibrary();
            return 1;
        }

        std::cout << "  Sent: " << dataSize << " bytes  [OK]" << std::endl;
    }

    // ----- Done -----
    closeSocket(sock);
    cleanupSocketLibrary();

    std::cout << "\n=== All " << numFiles << " file(s) sent successfully ===" << std::endl;
    return 0;
}
