# Danh Sách File Dự Án

## 📁 Cấu Trúc Thư Mục

```
SecureFileTransfer/
│
├─ include/des/              # DES header files
│  ├─ des_tables.h           - Khai báo bảng DES (IP, E, S-box, P, PC1, PC2)
│  ├─ des_core.h             - Khai báo hàm mã hóa / giải mã
│  └─ des_utils.h            - Khai báo file I/O, padding
│
├─ include/network/          # Network header files
│  └─ socket_utils.h         - Khai báo TCP socket wrapper
│
├─ src/des/                  # DES implementation
│  ├─ des_tables.cpp         - Định nghĩa bảng DES (~150 lines)
│  ├─ des_core.cpp           - Thuật toán DES Feistel 16 vòng (~250 lines)
│  └─ des_utils.cpp          - File I/O + PKCS#7 padding + UTF-8 support (~220 lines)
│
├─ src/network/              # Network implementation
│  └─ socket_utils.cpp       - TCP socket cross-platform (~300 lines)
│
├─ client_main.cpp           # Ứng dụng CLIENT (~260 lines)
├─ server_main.cpp           # Ứng dụng SERVER (~290 lines)
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
├─ Makefile                  - Build configuration (GNU Make)
├─ build-mingw.bat           - Windows batch build script
├─ .gitignore                - Git configuration
│
└─ Documentation/
   ├─ FILES.md               - This file
   ├─ RUN.md                 - How to build & run
   └─ ARCHITECTURE.md        - System design & protocol
```

---

## 📄 Giải Thích File

### Header Files

#### `include/des/des_tables.h`
- Khai báo các bảng DES chuẩn FIPS 46-3
- Export: `IP[64]`, `IP_INV[64]`, `E[48]`, `S_BOX[8][4][16]`, `P[32]`, `PC1[56]`, `PC2[48]`, `LEFT_SHIFTS[16]`

#### `include/des/des_core.h`
- Khai báo hàm mã hóa cốt lõi
- Export: `generateRoundKeys`, `encryptBlock`, `decryptBlock`, `encryptBlock3DES`, `decryptBlock3DES`, `desF`
- Type: `RoundKeys` (16 subkeys × 6 bytes)

#### `include/des/des_utils.h`
- Khai báo hàm file I/O và padding
- Export: `padData`, `unpadData`, `readFile`, `writeFile`
- Hỗ trợ tên file UTF-8 (tiếng Việt) trên Windows

#### `include/network/socket_utils.h`
- Khai báo TCP socket wrapper cross-platform
- Export: `createServerSocket`, `acceptConnection`, `connectToServer`, `sendData`, `receiveExact`

---

### Source Files

#### `src/des/des_tables.cpp` (~150 lines)
- Định nghĩa tất cả bảng DES theo chuẩn FIPS 46-3
- IP, IP_INV, E (expansion), S_BOX[8], P, PC1, PC2, LEFT_SHIFTS

#### `src/des/des_core.cpp` (~270 lines)
- Thuật toán DES 16-round Feistel Network và 3DES (Triple DES EDE3)
- `generateRoundKeys()` — Sinh 16 subkey từ 8-byte master key
- `desF()` — Hàm F: Expansion → XOR → S-box → Permutation
- `encryptBlock()` / `decryptBlock()` — Xử lý 1 block 64-bit bằng DES
- `encryptBlock3DES()` / `decryptBlock3DES()` — Xử lý block 64-bit bằng 3DES

#### `src/des/des_utils.cpp` (~220 lines)
- PKCS#7 padding / unpadding với validation
- `readFile()` / `writeFile()` — Binary I/O hỗ trợ UTF-8 filename  
  - Windows: dùng `_wfopen()` với wide string path
  - Linux/macOS: dùng `std::ifstream` / `std::ofstream`

#### `src/network/socket_utils.cpp` (~300 lines)
- TCP socket implementation cross-platform
- Windows: Winsock2 (`ws2_32`)
- Linux/macOS: POSIX sockets
- `receiveExact()` — Đảm bảo nhận đủ N bytes (blocking)

---

### Application Files

#### `client_main.cpp` (~260 lines)

**Nhiệm vụ:** Mã hóa và gửi nhiều file qua TCP trong 1 kết nối.

**Luồng:**
```
Parse UTF-8 args (CommandLineToArgvW)
→ Read & Encrypt all files (DES ECB, PKCS#7)
→ Connect TCP
→ Send [num_files]
→ For each file: Send [name_len][name][data_size][encrypted_data]
```

**Usage:**
```bash
client_send.exe <ip> <port> <key> <file1> [file2] ...
```

#### `server_main.cpp` (~290 lines)

**Nhiệm vụ:** Nhận nhiều file, giải mã, lưu đúng tên vào thư mục.

**Luồng:**
```
Listen TCP (accept 1 client)
→ Recv [num_files]
→ For each file:
    Recv [name_len][filename][data_size][ciphertext]
    → Decrypt (DES) + unpad (PKCS#7)
    → sanitizeFilename() (ngăn path traversal)
    → writeFile(outputDir/filename)
```

**Usage:**
```bash
server_recv.exe <port> <output_dir> <key>
```

---

### Build Configuration

#### `Makefile`
- GNU Make, tự động phát hiện OS
- Targets: `all`, `clean`, `clean-obj`, `run-server`, `run-client`, `info`, `help`
- Flags: `-std=c++11 -Wall -Wextra -O2`
- Link: `-lws2_32` (Windows Winsock2)

#### `build-mingw.bat`
- Windows batch script gọi Makefile với validation

---

## 📊 Thống Kê

| Component | Files | Lines | Nhiệm vụ |
|---|---|---|---|
| **DES Algorithm** | 6 | ~650 | Mã hóa / giải mã |
| **Network** | 2 | ~300 | TCP transport |
| **Application** | 2 | ~550 | Client / Server (DES & 3DES) |
| **Build** | 2 | ~220 | Compilation |
| **Tổng C++** | **12** | **~1,740** | |
| **GUI Python** | 1 | ~1000 | PyQt6 interface |
| **Documentation** | 3 | — | Hướng dẫn |

---

## 🔗 Dependencies

```
des_tables.h  ←── des_tables.cpp
des_core.h    ←── des_core.cpp  (uses des_tables.h)
des_utils.h   ←── des_utils.cpp (uses des_core.h, windows.h)
socket_utils.h ←── socket_utils.cpp (winsock2 / posix)

client_main.cpp  uses: des_core.h, des_utils.h, socket_utils.h, windows.h
server_main.cpp  uses: des_core.h, des_utils.h, socket_utils.h, windows.h
```

---

## 🖥️ GUI Files

### `GUI/app_pro.py` (~910 lines)

| Class | Mô tả |
|---|---|
| `TransferThread` | QThread chạy subprocess C++, lưu `proc` để kill khi đóng app |
| `MultiFileDropZone` | QFrame nhận drag-and-drop nhiều file |
| `FileListWidget` | Danh sách file gửi: hiển thị kích thước, xoá từng file |
| `DESTransferApp` | QMainWindow chính: 2 tab Gửi/Nhận, log panel, scroll |

---

**Updated**: April 2026
