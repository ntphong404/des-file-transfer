# Kiến Trúc DES File Transfer Pro

## 1. Tổng Quan Kiến Trúc

```
┌──────────────────────────────────────────────────────────────────────┐
│                     DES File Transfer Pro                            │
├────────────────────────────┬─────────────────────────────────────────┤
│   MÁY GỬI (Client)        │   MÁY NHẬN (Server)                     │
│                            │                                          │
│  ┌──────────────────────┐  │  ┌──────────────────────┐              │
│  │    GUI (PyQt6)       │  │  │    GUI (PyQt6)       │              │
│  │    app_pro.py        │  │  │    app_pro.py        │              │
│  └─────────┬────────────┘  │  └─────────┬────────────┘              │
│            │ subprocess    │             │ subprocess                 │
│  ┌─────────▼────────────┐  │  ┌─────────▼────────────┐              │
│  │  client_send.exe     │  │  │  server_recv.exe     │              │
│  │  client_main.cpp     │  │  │  server_main.cpp     │              │
│  └─────────┬────────────┘  │  └─────────┬────────────┘              │
│            │               │             │                            │
│  ┌─────────▼────────────┐  │  ┌─────────▼────────────┐              │
│  │  DES Layer           │  │  │  DES Layer           │              │
│  │  (des_core/utils)    │  │  │  (des_core/utils)    │              │
│  └─────────┬────────────┘  │  └─────────┬────────────┘              │
│            │               │             │                            │
│  ┌─────────▼────────────┐  │  ┌─────────▼────────────┐              │
│  │  Network Layer       │  │  │  Network Layer       │              │
│  │  (socket_utils)      ├──┼──►  (socket_utils)      │              │
│  └──────────────────────┘  │  └──────────────────────┘              │
│     TCP Multi-File ────────┼────────────────────────────────────►   │
└────────────────────────────┴─────────────────────────────────────────┘
```

## 2. Kiến Trúc Lớp (Layer Architecture)

### Layer 1: Application Layer

- **client_main.cpp**: Ứng dụng client, điều phối luồng chương trình
- **server_main.cpp**: Ứng dụng server, lắng nghe kết nối

### Layer 2: DES Cryptography Layer

- **des_core.h/cpp**: Mã hóa/giải mã 64-bit block
  - `generateRoundKeys()`: Sinh subkey từ master key
  - `encryptBlock()`: Mã hóa 1 block
  - `decryptBlock()`: Giải mã 1 block
- **des_utils.h/cpp**: Xử lý file toàn bộ
  - `padData()`: Thêm PKCS#7 padding
  - `unpadData()`: Bỏ padding
  - `encryptFile()`: Mã hóa file
  - `decryptFile()`: Giải mã file

- **des_tables.h/cpp**: Bảng DES tĩnh
  - IP, IP_INV, E-box, S-boxes, P-box
  - PC1, PC2, LEFT_SHIFTS

### Layer 3: Network Layer

- **socket_utils.h/cpp**: TCP Socket wrapper
  - Server: `createServerSocket()`, `acceptConnection()`
  - Client: `connectToServer()`
  - Data: `sendData()`, `receiveData()`

---

## 3. Luồng Dữ Liệu (Data Flow)

### Phía Client (Encrypt & Send — Multi-File)

```
Danh sách N file inputs
      │
      ▼
┌─────────────────────┐
│ getUtf8Args()       │  Đọc argv từ GetCommandLineW (UTF-8 safe)
└─────────────────────┘
      │
      ▼ Với mỗi file:
┌─────────────────────┐
│ readFile()          │  _wfopen (Windows UTF-8 filename)
└─────────────────────┘
      │ byte vector (plaintext)
      ▼
┌─────────────────────┐
│ padData (PKCS#7)    │  Align to 8-byte block boundary
└─────────────────────┘
      │
      ▼ Với mỗi 8-byte block:
┌─────────────────────┐
│ encryptBlock (DES)  │  IP → 16 Feistel rounds → FP
│ × (size/8) lần      │
└─────────────────────┘
      │ ciphertext vector
      ▼
┌─────────────────────────────────┐
│ sendUint32(num_files)           │  4 bytes
│ For each file:                  │
│   sendUint32(filename_len)      │  4 bytes
│   sendData(filename bytes)      │  UTF-8 string
│   sendUint64(data_size)         │  8 bytes
│   sendData(ciphertext)          │  N bytes
└─────────────────────────────────┘
      │
      ▼
  TCP ──────────────────────────────►
```

### Phía Server (Receive & Decrypt — Multi-File)

```
  ◄────────────────────────────── TCP
      │
      ▼
┌─────────────────────────────────┐
│ recvUint32(num_files)           │  Số lượng file
│ For each file:                  │
│   recvUint32(filename_len)      │
│   receiveExact(filename bytes)  │  Tên file UTF-8
│   recvUint64(data_size)         │
│   receiveExact(ciphertext)      │
└─────────────────────────────────┘
      │ ciphertext + filename
      ▼ Với mỗi 8-byte block:
┌─────────────────────┐
│ decryptBlock (DES)  │  IP → 16 Feistel rounds (reverse) → FP
│ × (size/8) lần      │
└─────────────────────┘
      │ padded plaintext
      ▼
┌─────────────────────┐
│ unpadData (PKCS#7)  │  Validate & remove padding
└─────────────────────┘
      │ plaintext
      ▼
┌─────────────────────────────────┐
│ sanitizeFilename()              │  Ngăn path traversal attack
│ writeFile(outDir/filename)      │  _wfopen (UTF-8 safe)
└─────────────────────────────────┘
      │
      ▼
File lưu vào output_dir với tên gốc
```

---

## 4. DES Algorithm Details

### Encrypt Block (Input: 64-bit plaintext, Key: 56-bit)

```
Plaintext (64-bit)
      │
      ↓ IP (Initial Permutation)
      │
      ├─ Left Half (32-bit)  [L0]
      └─ Right Half (32-bit) [R0]

For Round i = 1 to 16:
      │
      ├─ Li = Ri-1
      │
      ├─ Ri = Li-1 XOR F(Ri-1, Ki)
      │    where:
      │    F(R, K) = P(S-box(E(R) XOR K))
      │      ├─ E: Expansion (32→48 bits)
      │      ├─ XOR with RoundKey Ki
      │      ├─ S-box: Substitution (48→32 bits)
      │      └─ P: Permutation (32→32 bits)
      │

R16 || L16 (swap final round!)
      │
      ↓ FP (Final Permutation)
      │
Ciphertext (64-bit)
```

### Key Schedule

```
Master Key (64-bit) with parity
      │
      ↓ PC1 Permutation
      │
56-bit key split:
      ├─ C0 (Left 28 bits)
      └─ D0 (Right 28 bits)

For Round i = 1 to 16:
      │
      ├─ Rotate Ci-1 LEFT by Si positions → Ci
      │
      ├─ Rotate Di-1 LEFT by Si positions → Di
      │
      ├─ Concatenate: CiDi (56 bits)
      │
      ↓ PC2 Permutation (56→48 bits)
      │
      └─ RoundKey Ki (48 bits)

S values = [1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1]
```

---

## 5. Class Diagram

```
┌──────────────────────────────────────┐
│        DES Namespace                 │
├──────────────────────────────────────┤
│                                      │
│ CONSTANTS:                           │
│ - IP[64]                             │
│ - S_BOX[8][4][16]                    │
│ - PC1[56], PC2[48]                   │
│ - LEFT_SHIFTS[16]                    │
│                                      │
│ FUNCTIONS:                           │
│ + generateRoundKeys()                │
│ + encryptBlock()                     │
│ + decryptBlock()                     │
│ + desF()                             │
│ + padData()                          │
│ + unpadData()                        │
│ + readFile()                         │
│ + writeFile()                        │
│ + encryptFile()                      │
│ + decryptFile()                      │
└──────────────────────────────────────┘

┌──────────────────────────────────────┐
│      Network Namespace               │
├──────────────────────────────────────┤
│                                      │
│ TYPES:                               │
│ - SocketHandle                       │
│                                      │
│ FUNCTIONS:                           │
│ + initializeSocketLibrary()          │
│ + createServerSocket()               │
│ + acceptConnection()                 │
│ + connectToServer()                  │
│ + sendData()                         │
│ + receiveData()                      │
│ + closeSocket()                      │
└──────────────────────────────────────┘
```

---

## 6. Memory Layout

### Stack Frame (Client Main)

```
┌─────────────────────────────────────┐
│ Local Variables                     │
├─────────────────────────────────────┤
│ serverIP (string)        8 bytes    │
│ port (int)               4 bytes    │
│ inputFile (string)       8 bytes    │
│ keyStr (string)          8 bytes    │
│ desKey[8]                8 bytes    │
│ plaintext (vector)       24 bytes   │
│ padded (vector)          24 bytes   │
│ roundKeys               96 bytes    │
│ ciphertext (vector)      24 bytes   │
└─────────────────────────────────────┘
Total: ~212 bytes base + dynamic allocations
```

### Heap Allocations (Dynamic)

```
┌─────────────────────────────────────┐
│ std::vector<uint8_t> plaintext      │
│ - Size: file size                   │
│ - Capacity: allocated memory        │
├─────────────────────────────────────┤
│ std::vector<uint8_t> padded         │
│ - Size: multiple of 8               │
│ - Capacity: allocated memory        │
├─────────────────────────────────────┤
│ std::vector<uint8_t> ciphertext     │
│ - Size: padded size                 │
│ - Capacity: allocated memory        │
└─────────────────────────────────────┘
Total: 3 × file size (approximately)
```

---

## 7. Control Flow Diagram

### Client Main

```
START
  │
  ├─ Parse arguments
  │
  ├─ Initialize socket library
  │
  ├─ Read plaintext file
  │
  ├─ Apply PKCS#7 padding
  │
  ├─ Generate round keys from master key
  │
  ├─ Encrypt each 8-byte block
  │   └─ For each block:
  │      ├─ Apply IP
  │      ├─ 16 Feistel rounds
  │      └─ Apply FP
  │
  ├─ Connect to server
  │
  ├─ Send file size (8 bytes)
  │
  ├─ Send encrypted data
  │
  ├─ Close socket
  │
  ├─ Cleanup socket library
  │
  └─ EXIT
```

### Server Main

```
START
  │
  ├─ Parse arguments
  │
  ├─ Initialize socket library
  │
  ├─ Create server socket (bind, listen)
  │
  ├─ Accept client connection
  │
  ├─ Receive file size
  │
  ├─ Receive encrypted data in chunks
  │
  ├─ Generate round keys from master key
  │
  ├─ Decrypt each 8-byte block
  │   └─ For each block:
  │      ├─ Apply IP
  │      ├─ 16 Feistel rounds (REVERSE)
  │      └─ Apply FP
  │
  ├─ Remove PKCS#7 padding
  │
  ├─ Write plaintext to file
  │
  ├─ Close socket
  │
  ├─ Cleanup socket library
  │
  └─ EXIT
```

---

## 8. Error Handling Strategy

```
┌─────────────────────────────────────┐
│ Error Handling Hierarchy            │
├─────────────────────────────────────┤
│                                     │
│ Validation Layer:                   │
│ ├─ Check key length (must be 8)    │
│ ├─ Check port range (1-65535)      │
│ └─ Check file existence             │
│                                     │
│ Socket Layer:                       │
│ ├─ Check socket creation success    │
│ ├─ Check bind/listen success        │
│ ├─ Check connect success            │
│ ├─ Check send/recv success          │
│ └─ Handle network errors            │
│                                     │
│ Cryptography Layer:                 │
│ ├─ Check PKCS#7 padding validity   │
│ └─ Verify encryption/decryption    │
│                                     │
│ File I/O Layer:                     │
│ ├─ Check file open success          │
│ ├─ Check file read/write success    │
│ └─ Handle disk space errors         │
│                                     │
└─────────────────────────────────────┘
```

---

## 9. Summary

PROJECT: **DES Encryption with Network Transfer**

- **Type**: Cryptographic System + Network Application
- **Language**: C++17
- **Architecture**: Layered (App → Crypto → Network)
- **Platform**: Cross-platform (Windows/Linux/macOS)
- **Educational Purpose**: Demonstrate DES + socket programming
