/*
 * Author: PhongPKF
 * File: socket_utils.h
 * Description: Declares TCP socket wrapper functions for client-server communication.
 *              Simplifies socket operations: create, bind, listen, connect, send, receive.
 */

#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <cstdint>
#include <vector>
#include <string>

namespace Network
{

    // Socket handle type (platform-independent wrapper)
    using SocketHandle = int;

    // Invalid socket identifier
    static constexpr SocketHandle INVALID_SOCKET = -1;

    // Maximum number of pending connections
    static constexpr int MAX_PENDING_CONNECTIONS = 5;

    // Buffer size for data transmission (64 KB)
    static constexpr size_t BUFFER_SIZE = 65536;

    /*
     * Initialize socket library (Windows: WSAStartup)
     * Must be called before using any socket functions
     */
    bool initializeSocketLibrary();

    /*
     * Cleanup socket library (Windows: WSACleanup)
     * Must be called when done with socket operations
     */
    void cleanupSocketLibrary();

    /*
     * Create a server socket and start listening
     * Input:  port number
     * Output: socket handle, or INVALID_SOCKET on failure
     */
    SocketHandle createServerSocket(uint16_t port);

    /*
     * Accept incoming client connection
     * Input:  server socket handle
     * Output: connected client socket handle
     */
    SocketHandle acceptConnection(SocketHandle serverSocket);

    /*
     * Connect to remote server
     * Input:  server IP address (e.g., "127.0.0.1"), port number
     * Output: socket handle, or INVALID_SOCKET on failure
     */
    SocketHandle connectToServer(const std::string &serverIP, uint16_t port);

    /*
     * Send data through socket with error handling
     * Input:  socket handle, data buffer, data length
     * Output: true if all data was sent successfully
     */
    bool sendData(SocketHandle sock, const uint8_t *buffer, size_t length);

    /*
     * Receive data from socket with error handling
     * Input:  socket handle, buffer to store received data
     * Output: number of bytes received (0 if connection closed, -1 on error)
     */
    int receiveData(SocketHandle sock, std::vector<uint8_t> &buffer);

    /*
     * Receive exactly N bytes from socket (blocking until all received)
     * Input:  socket handle, buffer to store received data, exact number of bytes to receive
     * Output: number of bytes received (should equal count on success, -1 on error)
     */
    int receiveExact(SocketHandle sock, std::vector<uint8_t> &buffer, size_t count);

    /*
     * Send entire file through socket
     * Input:  socket handle, file path
     * Output: true if successful
     */
    bool sendFile(SocketHandle sock, const std::string &filePath);

    /*
     * Receive entire file through socket
     * Input:  socket handle, output file path
     * Output: true if successful
     */
    bool receiveFile(SocketHandle sock, const std::string &outputPath);

    /*
     * Close socket connection
     * Input:  socket handle
     */
    void closeSocket(SocketHandle sock);

    /*
     * Get error message from system error code
     * Output: human-readable error message
     */
    std::string getLastErrorMessage();

} // namespace Network

#endif // SOCKET_UTILS_H
