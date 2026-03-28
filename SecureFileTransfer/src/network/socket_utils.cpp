/*
 * Author: PhongPKF
 * File: socket_utils.cpp
 * Description: Implements TCP socket wrapper functions for cross-platform (Windows/Linux)
 *              client-server communication with error handling.
 */

#include "../../include/network/socket_utils.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace Network
{

    static bool socketLibraryInitialized = false;

    // Initialize socket library
    bool initializeSocketLibrary()
    {
#ifdef _WIN32
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            std::cerr << "WSAStartup failed with error code: " << result << std::endl;
            return false;
        }
        socketLibraryInitialized = true;
        return true;
#else
        socketLibraryInitialized = true;
        return true;
#endif
    }

    // Cleanup socket library
    void cleanupSocketLibrary()
    {
#ifdef _WIN32
        if (socketLibraryInitialized)
        {
            WSACleanup();
            socketLibraryInitialized = false;
        }
#endif
    }

    // Create server socket
    SocketHandle createServerSocket(uint16_t port)
    {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

#ifdef _WIN32
        SocketHandle serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET)
        {
            std::cerr << "Socket creation error. Code: " << WSAGetLastError() << std::endl;
            return INVALID_SOCKET;
        }

        // Allow reusing address
        int reuse = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
        {
            std::cerr << "setsockopt error" << std::endl;
            closesocket(serverSocket);
            return INVALID_SOCKET;
        }

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            std::cerr << "Bind failed with error code: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            return INVALID_SOCKET;
        }

        if (listen(serverSocket, MAX_PENDING_CONNECTIONS) == SOCKET_ERROR)
        {
            std::cerr << "Listen failed with error code: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            return INVALID_SOCKET;
        }

        std::cout << "Server listening on port " << port << std::endl;
        return serverSocket;
#else
        SocketHandle serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0)
        {
            std::cerr << "Socket creation error" << std::endl;
            return INVALID_SOCKET;
        }

        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&1, sizeof(int)) < 0)
        {
            std::cerr << "setsockopt error" << std::endl;
            close(serverSocket);
            return INVALID_SOCKET;
        }

        if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            std::cerr << "Bind error" << std::endl;
            close(serverSocket);
            return INVALID_SOCKET;
        }

        if (listen(serverSocket, MAX_PENDING_CONNECTIONS) < 0)
        {
            std::cerr << "Listen error" << std::endl;
            close(serverSocket);
            return INVALID_SOCKET;
        }

        std::cout << "Server listening on port " << port << std::endl;
        return serverSocket;
#endif
    }

    // Accept connection
    SocketHandle acceptConnection(SocketHandle serverSocket)
    {
#ifdef _WIN32
        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SocketHandle clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Accept failed with error code: " << WSAGetLastError() << std::endl;
            return INVALID_SOCKET;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        return clientSocket;
#else
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        SocketHandle clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket < 0)
        {
            std::cerr << "Accept error" << std::endl;
            return INVALID_SOCKET;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        return clientSocket;
#endif
    }

    // Connect to server
    SocketHandle connectToServer(const std::string &serverIP, uint16_t port)
    {
#ifdef _WIN32
        SocketHandle clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Socket creation error: " << WSAGetLastError() << std::endl;
            return INVALID_SOCKET;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            std::cerr << "Connection failed with error code: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            return INVALID_SOCKET;
        }

        std::cout << "Connected to server " << serverIP << ":" << port << std::endl;
        return clientSocket;
#else
        SocketHandle clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            std::cerr << "Socket creation error" << std::endl;
            return INVALID_SOCKET;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            std::cerr << "Connection failed" << std::endl;
            close(clientSocket);
            return INVALID_SOCKET;
        }

        std::cout << "Connected to server " << serverIP << ":" << port << std::endl;
        return clientSocket;
#endif
    }

    // Send data
    bool sendData(SocketHandle sock, const uint8_t *buffer, size_t length)
    {
        size_t sent = 0;
        while (sent < length)
        {
            int toSend = (int)std::min(BUFFER_SIZE, length - sent);
#ifdef _WIN32
            int result = send(sock, (const char *)(buffer + sent), toSend, 0);
            if (result == SOCKET_ERROR)
            {
                std::cerr << "Send failed with error code: " << WSAGetLastError() << std::endl;
                return false;
            }
#else
            int result = send(sock, buffer + sent, toSend, 0);
            if (result < 0)
            {
                std::cerr << "Send error: " << strerror(errno) << std::endl;
                return false;
            }
#endif
            sent += result;
        }
        return true;
    }

    // Receive data
    int receiveData(SocketHandle sock, std::vector<uint8_t> &buffer)
    {
        buffer.resize(BUFFER_SIZE);
#ifdef _WIN32
        int result = recv(sock, (char *)buffer.data(), BUFFER_SIZE, 0);
        if (result == SOCKET_ERROR)
        {
            std::cerr << "Recv failed with error code: " << WSAGetLastError() << std::endl;
            return -1;
        }
#else
        int result = recv(sock, buffer.data(), BUFFER_SIZE, 0);
        if (result < 0)
        {
            std::cerr << "Recv error: " << strerror(errno) << std::endl;
            return -1;
        }
#endif
        buffer.resize(result);
        return result;
    }

    // Receive exactly count bytes (blocking)
    int receiveExact(SocketHandle sock, std::vector<uint8_t> &buffer, size_t count)
    {
        buffer.resize(count);
        size_t totalReceived = 0;

        while (totalReceived < count)
        {
            int chunk = std::min((size_t)65536, count - totalReceived);
#ifdef _WIN32
            int result = recv(sock, (char *)(buffer.data() + totalReceived), chunk, 0);
            if (result == SOCKET_ERROR)
            {
                std::cerr << "Recv failed with error code: " << WSAGetLastError() << std::endl;
                return -1;
            }
            if (result == 0)
            {
                std::cerr << "Connection closed by peer" << std::endl;
                return (int)totalReceived;
            }
#else
            int result = recv(sock, buffer.data() + totalReceived, chunk, 0);
            if (result < 0)
            {
                std::cerr << "Recv error: " << strerror(errno) << std::endl;
                return -1;
            }
            if (result == 0)
            {
                std::cerr << "Connection closed by peer" << std::endl;
                return (int)totalReceived;
            }
#endif
            totalReceived += result;
        }

        return (int)totalReceived;
    }

    // Send file
    bool sendFile(SocketHandle sock, const std::string &filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open file " << filePath << std::endl;
            return false;
        }

        // Get file size
        file.seekg(0, std::ios::end);
        uint64_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Send file size (8 bytes)
        uint8_t sizeBuffer[8];
        for (int i = 7; i >= 0; i--)
        {
            sizeBuffer[i] = (uint8_t)(fileSize & 0xFF);
            fileSize >>= 8;
        }
        if (!sendData(sock, sizeBuffer, 8))
        {
            file.close();
            return false;
        }

        // Send file content
        std::vector<uint8_t> buffer(BUFFER_SIZE);
        while (file.read((char *)buffer.data(), BUFFER_SIZE) || file.gcount() > 0)
        {
            if (!sendData(sock, buffer.data(), file.gcount()))
            {
                file.close();
                return false;
            }
        }

        file.close();
        return true;
    }

    // Receive file
    bool receiveFile(SocketHandle sock, const std::string &outputPath)
    {
        // Receive file size
        std::vector<uint8_t> sizeBuffer(8);
        int bytesRecv = receiveData(sock, sizeBuffer);
        if (bytesRecv != 8)
        {
            std::cerr << "Error: Failed to receive file size" << std::endl;
            return false;
        }

        uint64_t fileSize = 0;
        for (int i = 0; i < 8; i++)
        {
            fileSize = (fileSize << 8) | sizeBuffer[i];
        }

        std::cout << "Receiving file of size: " << fileSize << " bytes" << std::endl;

        // Receive file content
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error: Cannot open file " << outputPath << " for writing" << std::endl;
            return false;
        }

        uint64_t totalRecv = 0;
        std::vector<uint8_t> buffer;
        while (totalRecv < fileSize)
        {
            int bytesRecv = receiveData(sock, buffer);
            if (bytesRecv <= 0)
            {
                std::cerr << "Connection closed while receiving file" << std::endl;
                file.close();
                return false;
            }
            file.write((const char *)buffer.data(), bytesRecv);
            totalRecv += bytesRecv;
        }

        file.close();
        std::cout << "File received successfully" << std::endl;
        return true;
    }

    // Close socket
    void closeSocket(SocketHandle sock)
    {
#ifdef _WIN32
        if (sock != INVALID_SOCKET)
        {
            closesocket(sock);
        }
#else
        if (sock != INVALID_SOCKET)
        {
            close(sock);
        }
#endif
    }

    // Get last error message
    std::string getLastErrorMessage()
    {
#ifdef _WIN32
        int errorCode = WSAGetLastError();
        char *errorMsg = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&errorMsg, 0, NULL);
        std::string result(errorMsg ? errorMsg : "Unknown error");
        if (errorMsg)
            LocalFree(errorMsg);
        return result;
#else
        return std::string(strerror(errno));
#endif
    }

} // namespace Network
