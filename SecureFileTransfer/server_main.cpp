/*
 * Author: PhongPKF
 * File: server_main.cpp
 * Description: Server - nhận NHIỀU file kèm tên file, giải mã DES, lưu vào thư mục.
 *
 * Protocol (mới):
 *   [4B: num_files]
 *   For each file:
 *     [4B: filename_len] [filename_len bytes: filename]
 *     [8B: encrypted_data_size] [encrypted_data bytes]
 *
 * Usage: server_recv.exe <port> <output_dir> <key>
 */

#include "include/des/des_core.h"
#include "include/des/des_utils.h"
#include "include/network/socket_utils.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>   // _mkdir
#include <windows.h>  // CreateDirectoryA
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace DES;
using namespace Network;

void printUsage(const char *programName)
{
    std::cout << "Usage: " << programName << " <port> <output_dir> <key>" << std::endl;
    std::cout << "Example: " << programName << " 5000 ./received 12345678" << std::endl;
    std::cout << "Note: Files are saved with their original names inside <output_dir>" << std::endl;
}

// Parse key: allow 8 bytes for DES, 24 bytes for 3DES
bool parseKey(const std::string &keyStr, uint8_t *key, bool &is3DES)
{
    if (keyStr.length() >= 24)
    {
        std::memcpy(key, keyStr.c_str(), 24);
        is3DES = true;
        return true;
    }
    else if (keyStr.length() >= 8)
    {
        std::memcpy(key, keyStr.c_str(), 8);
        is3DES = false;
        return true;
    }
    std::cerr << "Error: Key must be at least 8 characters (or 24 for 3DES)" << std::endl;
    return false;
}

// Receive uint32_t big-endian (4 bytes)
bool recvUint32(SocketHandle sock, uint32_t &value)
{
    std::vector<uint8_t> buf;
    int n = receiveExact(sock, buf, 4);
    if (n != 4) return false;
    value = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
          | ((uint32_t)buf[2] << 8)  |  (uint32_t)buf[3];
    return true;
}

// Receive uint64_t big-endian (8 bytes)
bool recvUint64(SocketHandle sock, uint64_t &value)
{
    std::vector<uint8_t> buf;
    int n = receiveExact(sock, buf, 8);
    if (n != 8) return false;
    value = 0;
    for (int i = 0; i < 8; i++)
        value = (value << 8) | buf[i];
    return true;
}

// Convert UTF-8 → wstring (dùng chung)
static std::wstring utf8ToWide(const std::string &s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

#ifdef _WIN32
// Lấy args đúng từ GetCommandLineW() → UTF-8 (hỗ trợ tiếng Việt)
static std::vector<std::string> getUtf8Args()
{
    int wargc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    std::vector<std::string> args;
    for (int i = 0; i < wargc; i++)
    {
        int sz = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
        std::string s(sz - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, &s[0], sz, nullptr, nullptr);
        args.push_back(s);
    }
    LocalFree(wargv);
    return args;
}
#endif

// Ensure a directory exists (create if not present)
void ensureDirectory(const std::string &path)
{
#ifdef _WIN32
    CreateDirectoryW(utf8ToWide(path).c_str(), NULL);
#else
    mkdir(path.c_str(), 0755);
#endif
}

// Sanitize filename: strip path separators to prevent directory traversal
std::string sanitizeFilename(const std::string &name)
{
    std::string safe = name;
    for (char &c : safe)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    }
    if (safe.empty()) safe = "unnamed";
    return safe;
}

// Join directory and filename into a path
std::string joinPath(const std::string &dir, const std::string &file)
{
    if (dir.empty()) return file;
    char last = dir.back();
    if (last == '/' || last == '\\')
        return dir + file;
    return dir + "/" + file;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printUsage(argv[0]);
        return 1;
    }

#ifdef _WIN32
    auto args = getUtf8Args();
    int port              = std::atoi(args[1].c_str());
    std::string outputDir = args[2];
    std::string keyStr    = args[3];
#else
    int port = std::atoi(argv[1]);
    std::string outputDir = argv[2];
    std::string keyStr    = argv[3];
#endif


    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
        return 1;
    }

    uint8_t desKey[24];
    bool is3DES = false;
    if (!parseKey(keyStr, desKey, is3DES))
        return 1;

    std::cout << "=== " << (is3DES ? "3DES" : "DES") << " Multi-File Reception & Decryption (Server) ===" << std::endl;
    std::cout << "Port      : " << port << std::endl;
    std::cout << "Mode      : " << (is3DES ? "3DES (24-byte key) EDE3" : "DES (8-byte key)") << std::endl;
    std::cout << "Output dir: " << outputDir << std::endl;

    if (!initializeSocketLibrary())
    {
        std::cerr << "Failed to initialize socket library" << std::endl;
        return 1;
    }

    // Ensure output directory exists
    ensureDirectory(outputDir);

    // Create server socket
    std::cout << "\n[1] Creating server socket..." << std::endl;
    SocketHandle serverSock = createServerSocket((uint16_t)port);
    if (serverSock == INVALID_SOCKET)
    {
        cleanupSocketLibrary();
        return 1;
    }

    // Accept one client
    std::cout << "[2] Waiting for client connection..." << std::endl;
    SocketHandle clientSock = acceptConnection(serverSock);
    closeSocket(serverSock); // no longer needed

    if (clientSock == INVALID_SOCKET)
    {
        cleanupSocketLibrary();
        return 1;
    }

    // Generate round keys
    RoundKeys rk1, rk2, rk3;
    if (is3DES) {
        generateRoundKeys(desKey, rk1);
        generateRoundKeys(desKey + 8, rk2);
        generateRoundKeys(desKey + 16, rk3);
    } else {
        generateRoundKeys(desKey, rk1);
    }

    // ----- Receive number of files -----
    uint32_t numFiles = 0;
    if (!recvUint32(clientSock, numFiles))
    {
        std::cerr << "Failed to receive file count" << std::endl;
        closeSocket(clientSock);
        cleanupSocketLibrary();
        return 1;
    }
    std::cout << "[3] Expecting " << numFiles << " file(s)..." << std::endl;

    int successCount = 0;

    for (uint32_t i = 0; i < numFiles; i++)
    {
        std::cout << "\n--- File [" << (i + 1) << "/" << numFiles << "] ---" << std::endl;

        // 1) filename length
        uint32_t nameLen = 0;
        if (!recvUint32(clientSock, nameLen) || nameLen == 0 || nameLen > 4096)
        {
            std::cerr << "  Invalid filename length: " << nameLen << std::endl;
            break;
        }

        // 2) filename
        std::vector<uint8_t> nameBuf;
        int nr = receiveExact(clientSock, nameBuf, nameLen);
        if (nr != (int)nameLen)
        {
            std::cerr << "  Failed to receive filename" << std::endl;
            break;
        }
        std::string filename(nameBuf.begin(), nameBuf.end());
        std::cout << "  Filename  : " << filename << std::endl;

        // 3) encrypted data size
        uint64_t dataSize = 0;
        if (!recvUint64(clientSock, dataSize))
        {
            std::cerr << "  Failed to receive data size" << std::endl;
            break;
        }
        std::cout << "  Data size : " << dataSize << " bytes (encrypted)" << std::endl;

        // 4) encrypted data
        std::vector<uint8_t> ciphertext;
        int recvResult = receiveExact(clientSock, ciphertext, (size_t)dataSize);
        if (recvResult != (int)dataSize)
        {
            std::cerr << "  Data incomplete: expected " << dataSize
                      << ", got " << recvResult << std::endl;
            break;
        }

        // Validate ciphertext alignment
        if (ciphertext.size() % BLOCK_SIZE != 0)
        {
            std::cerr << "  Ciphertext size not multiple of block size!" << std::endl;
            break;
        }

        // Decrypt
        std::vector<uint8_t> decrypted;
        decrypted.reserve(ciphertext.size());
        for (size_t j = 0; j < ciphertext.size(); j += BLOCK_SIZE)
        {
            uint8_t block[BLOCK_SIZE];
            if (is3DES) {
                decryptBlock3DES(ciphertext.data() + j, block, rk1, rk2, rk3);
            } else {
                decryptBlock(ciphertext.data() + j, block, rk1);
            }
            decrypted.insert(decrypted.end(), block, block + BLOCK_SIZE);
        }

        // Remove PKCS#7 padding
        if (!unpadData(decrypted))
        {
            std::cerr << "  Invalid PKCS#7 padding - wrong key or corrupted data!" << std::endl;
            break;
        }
        std::cout << "  Decrypted : " << decrypted.size() << " bytes" << std::endl;

        // Save file with sanitized original name
        std::string safeFilename = sanitizeFilename(filename);
        std::string outputPath   = joinPath(outputDir, safeFilename);

        if (writeFile(outputPath, decrypted))
        {
            std::cout << "  Saved to  : " << outputPath << "  [OK]" << std::endl;
            successCount++;
        }
        else
        {
            std::cerr << "  Failed to write: " << outputPath << std::endl;
        }
    }

    closeSocket(clientSock);
    cleanupSocketLibrary();

    std::cout << "\n=== Result: " << successCount << "/" << numFiles
              << " files received successfully ===" << std::endl;

    return (successCount == (int)numFiles) ? 0 : 1;
}
