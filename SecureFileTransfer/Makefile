# Makefile for SecureFileTransfer DES Project
# Windows MinGW + GNU Make
# Compiler: g++, GCC 5.0+
# Usage: make [all|clean|clean-obj|run-server|run-client|info|help]

# ========================
# COMPILER & FLAGS
# ========================
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
INCLUDES = -I./include

# ========================
# DIRECTORIES
# ========================
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
OBJ_DIR = obj

# ========================
# SOURCE & OBJECT FILES
# ========================
# DES Core
DES_CORE_SRC = $(SRC_DIR)/des/des_core.cpp
DES_CORE_OBJ = $(OBJ_DIR)/des_core.o

# DES Tables
DES_TABLES_SRC = $(SRC_DIR)/des/des_tables.cpp
DES_TABLES_OBJ = $(OBJ_DIR)/des_tables.o

# DES Utils (File I/O)
DES_UTILS_SRC = $(SRC_DIR)/des/des_utils.cpp
DES_UTILS_OBJ = $(OBJ_DIR)/des_utils.o

# Network
SOCKET_SRC = $(SRC_DIR)/network/socket_utils.cpp
SOCKET_OBJ = $(OBJ_DIR)/socket_utils.o

# Main programs
CLIENT_SRC = client_main.cpp
CLIENT_OBJ = $(OBJ_DIR)/client_main.o
CLIENT_BIN = $(BIN_DIR)/client_send.exe

SERVER_SRC = server_main.cpp
SERVER_OBJ = $(OBJ_DIR)/server_main.o
SERVER_BIN = $(BIN_DIR)/server_recv.exe

# Common objects
COMMON_OBJS = $(DES_CORE_OBJ) $(DES_TABLES_OBJ) $(DES_UTILS_OBJ) $(SOCKET_OBJ)

# ========================
# TARGETS
# ========================

# Default target
.PHONY: all
all: $(CLIENT_BIN) $(SERVER_BIN)
	@echo ======================================
	@echo Build successful!
	@echo ======================================
	@echo.
	@echo Executables created:
	@echo  - $(CLIENT_BIN)
	@echo  - $(SERVER_BIN)
	@echo.
	@echo Run server: bin\server_recv.exe 5000 output.txt 12345678
	@echo Run client: bin\client_send.exe 127.0.0.1 5000 data\plain.txt 12345678

# Create directories
$(BIN_DIR):
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)

$(OBJ_DIR):
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

# ========================
# COMPILE: DES Core
# ========================
$(DES_CORE_OBJ): $(DES_CORE_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(DES_CORE_SRC) -o $@

# ========================
# COMPILE: DES Tables
# ========================
$(DES_TABLES_OBJ): $(DES_TABLES_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(DES_TABLES_SRC) -o $@

# ========================
# COMPILE: DES Utils
# ========================
$(DES_UTILS_OBJ): $(DES_UTILS_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(DES_UTILS_SRC) -o $@

# ========================
# COMPILE: Socket Utils
# ========================
$(SOCKET_OBJ): $(SOCKET_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SOCKET_SRC) -o $@

# ========================
# COMPILE: Client Main
# ========================
$(CLIENT_OBJ): $(CLIENT_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(CLIENT_SRC) -o $@

# ========================
# COMPILE: Server Main
# ========================
$(SERVER_OBJ): $(SERVER_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $(SERVER_SRC) -o $@

# ========================
# LINK: Client Executable
# ========================
$(CLIENT_BIN): $(CLIENT_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lws2_32
	@echo [✓] Client compiled: $(CLIENT_BIN)

# ========================
# LINK: Server Executable
# ========================
$(SERVER_BIN): $(SERVER_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lws2_32
	@echo [✓] Server compiled: $(SERVER_BIN)

# ========================
# UTILITY TARGETS
# ========================

# Clean everything
.PHONY: clean
clean: clean-obj
	@echo Removing $(BIN_DIR)...
	@if exist $(BIN_DIR) rmdir /Q /S $(BIN_DIR) 2>nul || true
	@echo [✓] Clean complete

# Clean only object files
.PHONY: clean-obj
clean-obj:
	@echo Removing $(OBJ_DIR)...
	@if exist $(OBJ_DIR) rmdir /Q /S $(OBJ_DIR) 2>nul || true
	@echo [✓] Object files removed

# Info
.PHONY: info
info:
	@echo ======================================
	@echo Project: SecureFileTransfer DES
	@echo ======================================
	@echo.
	@echo Compiler Information:
	@g++ --version 2>&1 | findstr /R ".*" | findstr /E "^g"
	@make --version 2>&1 | findstr /R "GNU Make" 
	@echo.
	@echo Build Targets:
	@echo  - CLIENT: $(CLIENT_BIN)
	@echo  - SERVER: $(SERVER_BIN)
	@echo.
	@echo Source Files:
	@echo  - $(DES_CORE_SRC)
	@echo  - $(DES_TABLES_SRC)
	@echo  - $(DES_UTILS_SRC)
	@echo  - $(SOCKET_SRC)
	@echo  - $(CLIENT_SRC)
	@echo  - $(SERVER_SRC)

# Help
.PHONY: help
help:
	@echo ======================================
	@echo GNU Make Targets
	@echo ======================================
	@echo.
	@echo make all               - Build client & server [DEFAULT]
	@echo make clean             - Remove bin/ and obj/
	@echo make clean-obj         - Remove only obj/
	@echo make info              - Show compiler info
	@echo make help              - Show this help
	@echo make run-server        - Run server (requires bin/server_recv.exe)
	@echo make run-client        - Run client (requires bin/client_send.exe)
	@echo.
	@echo Examples:
	@echo   make clean
	@echo   make all
	@echo   make run-server
	@echo   make run-client

# ========================
# RUN TARGETS (Optional)
# ========================

.PHONY: run-server
run-server: $(SERVER_BIN)
	@echo Starting Server on port 5000...
	@echo Key: 12345678
	@$(SERVER_BIN) 5000 output.txt 12345678

.PHONY: run-client
run-client: $(CLIENT_BIN)
	@echo Sending file from client...
	@echo Target: 127.0.0.1:5000
	@echo Key: 12345678
	@$(CLIENT_BIN) 127.0.0.1 5000 data\plain.txt 12345678

.PHONY: test
test:
	@echo Testing DES project...
	@echo [1] Removing old build...
	@make clean-obj
	@echo.
	@echo [2] Building project...
	@make all
	@echo.
	@echo [3] Build complete! Ready to run:
	@echo    Terminal 1: make run-server
	@echo    Terminal 2: make run-client

# Mark all targets as PHONY (not file-dependent)
.PHONY: all clean clean-obj info help run-server run-client test
