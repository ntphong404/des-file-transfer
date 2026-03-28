# Danh Sách File Dự Án

## 📁 Cấu Trúc Thư Mục

```
SecureFileTransfer/
│
├─ include/des/              # DES header files
│  ├─ des_tables.h           - Khai báo bảng DES
│  ├─ des_core.h             - Khai báo hàm mã hóa
│  └─ des_utils.h            - Khai báo file I/O
│
├─ include/network/          # Network header files
│  └─ socket_utils.h         - Khai báo TCP socket
│
├─ src/des/                  # DES implementation
│  ├─ des_tables.cpp         - Định nghĩa bảng (150 lines)
│  ├─ des_core.cpp           - Thuật toán DES (250 lines)
│  └─ des_utils.cpp          - File I/O + padding (150 lines)
│
├─ src/network/              # Network implementation
│  └─ socket_utils.cpp       - Socket wrapper (300 lines)
│
├─ client_main.cpp           # Ứng dụng CLIENT (150 lines)
├─ server_main.cpp           # Ứng dụng SERVER (200 lines)
│
├─ bin/                      # Compiled executables
│  ├─ client_send.exe        - Chạy trên máy gửi
│  └─ server_recv.exe        - Chạy trên máy nhận
│
├─ obj/                      # Object files (tạo khi build)
│
├─ data/
│  └─ plain.txt              - File test (~800 bytes)
│
├─ Makefile                  # Build configuration (GNU Make)
├─ build-mingw.bat           - Windows batch script build
├─ .gitignore                - Git configuration
│
└─ Documentation/
   ├─ FILES.md               - This file (file listing)
   ├─ RUN.md                 - How to build & run
   ├─ ARCHITECTURE.md        - System design & flow
   └─ (Xem README.md ở ngoài cùng cho tổng quan)
```

---

## 📄 Giải Thích File

### Header Files (Khai Báo)

#### `include/des/des_tables.h`

- **Mục đích**: Khai báo các bảng DES
- **Xuất**: IP, S_BOX[8][4][16], P, PC1, PC2, LEFT_SHIFTS
- **Dùng cho**: Hoán vị, thay thế bit

#### `include/des/des_core.h`

- **Mục đích**: Khai báo hàm mã hóa cốt lõi
- **Xuất**: generateRoundKeys, encryptBlock, decryptBlock, desF
- **Dùng cho**: Mã hóa/giải mã block

#### `include/des/des_utils.h`

- **Mục đích**: Khai báo hàm file I/O
- **Xuất**: padData, unpadData, readFile, writeFile, encryptFile, decryptFile
- **Dùng cho**: Xử lý file và padding

#### `include/network/socket_utils.h`

- **Mục đích**: Khai báo socket TCP wrapper
- **Xuất**: createServerSocket, connectToServer, sendData, receiveData
- **Dùng cho**: Gửi nhận dữ liệu qua mạng

---

### Source Files (Triển Khai)

#### `src/des/des_tables.cpp` (~150 lines)

- Định nghĩa tất cả bảng DES (FIPS 46-3 standard)
- IP[64], S_BOX[8][4][16], E[48], P[32], PC1[56], PC2[48]
- Các hằng số LEFT_SHIFTS[16]

#### `src/des/des_core.cpp` (~250 lines)

- Triển khai thuật toán DES 16-round Feistel
- **generateRoundKeys()** - Sinh 16 subkey từ master key
- **desF()** - Hàm F (phức tạp nhất)
- **encryptBlock()** - Mã hóa 1 block 64-bit
- **decryptBlock()** - Giải mã 1 block 64-bit
- Helper: getBit, permute

#### `src/des/des_utils.cpp` (~150 lines)

- PKCS#7 padding/unpadding
- readFile, writeFile (binary I/O)
- encryptFile, decryptFile (ECB mode)

#### `src/network/socket_utils.cpp` (~300 lines)

- TCP socket implementation
- Windows (Winsock2) & Unix (POSIX sockets)
- Server: createServerSocket, acceptConnection
- Client: connectToServer
- Data: sendData, receiveData
- Auto platform detection

---

### Application Files

#### `client_main.cpp` (~150 lines)

- **Nhiệm vụ**: Gửi file được mã hóa
- **Luồng**: Read → Encrypt → Send
- **I/O**:
  - Input: data/plain.txt
  - Output: Encrypted bytes qua TCP

#### `server_main.cpp` (~200 lines)

- **Nhiệm vụ**: Nhận và giải mã file
- **Luồng**: Listen → Receive → Decrypt → Save
- **I/O**:
  - Input: Encrypted bytes từ TCP
  - Output: output.txt (plaintext)

---

### Build Configuration

#### `Makefile`

- GNU Make configuration
- Tự động phát hiện OS (Windows/Linux/macOS)
- Targets: all, clean, run-server, run-client, info

#### `CMakeLists.txt`

- CMake configuration (backup, không cần nếu dùng Make)
- Hỗ trợ Windows/Linux/macOS

#### `build-mingw.bat`

- Windows batch script
- Gọi Makefile với validation

---

### Test Data

#### `data/plain.txt` (~800 bytes)

- File test mẫu
- Nội dung: Mô tả project + DES algorithm
- Format: UTF-8 text

---

## 📊 Thống Kê

| Component         | File     | Lines     | Purpose               |
| ----------------- | -------- | --------- | --------------------- |
| **DES Logic**     | 6 files  | 1,150     | Encryption/decryption |
| **Network**       | 2 files  | 400       | TCP communication     |
| **Application**   | 2 files  | 350       | Client/server apps    |
| **Build**         | 3 files  | 280       | Compilation           |
| **Total Code**    | 13 files | **2,180** | Core logic            |
| **Documentation** | 4 files  | -         | User guides           |
| **Test Data**     | 1 file   | -         | Sample test file      |

---

## 🔗 Dependencies

### Header to Implementation

```
des_tables.h  ← des_tables.cpp
des_core.h    ← des_core.cpp + des_tables.h
des_utils.h   ← des_utils.cpp + des_core.h
socket_utils.h ← socket_utils.cpp
```

### Application to Libraries

```
client_main.cpp  uses: des_core.h, des_utils.h, socket_utils.h
server_main.cpp  uses: des_core.h, des_utils.h, socket_utils.h
```

### Platform Headers

```
Windows: #include <winsock2.h>
Linux:   #include <arpa/inet.h>
```

---

## 🎯 Mỗi File Làm Gì

| File                 | Input           | Process      | Output           |
| -------------------- | --------------- | ------------ | ---------------- |
| **client_main.cpp**  | plain.txt       | Encrypt      | TCP data         |
| **des_core.cpp**     | Plaintext block | 16 rounds    | Ciphertext block |
| **socket_utils.cpp** | Bytes           | Send/Receive | TCP transfer     |
| **server_main.cpp**  | TCP data        | Decrypt      | output.txt       |

---

## 📝 Kích Thước File

```
Header files:     ~350 bytes total
Source files:     ~15 KB (compiled ~2-3 MB dengan debug symbols)
Executables:      ~2.5 MB each (client + server)
Object files:     ~100 KB total (during build)
Static libs:      ~100 KB total (libdes.a + libnetwork.a)
```

---

**Total: 22 files, ~2,200 lines code, ~3,500 lines documentation**
