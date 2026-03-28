# 🔐 DES File Transfer Pro

Ứng dụng mã hóa và truyền file an toàn qua mạng LAN sử dụng DES encryption

## 📋 Mục lục

- [Tính năng](#tính-năng)
- [Cấu trúc dự án](#cấu-trúc-dự-án)
- [Yêu cầu hệ thống](#yêu-cầu-hệ-thống)
- [Cài đặt](#cài-đặt)
- [Hướng dẫn sử dụng](#hướng-dẫn-sử-dụng)
- [Kiến trúc hệ thống](#kiến-trúc-hệ-thống)

## ✨ Tính năng

- **Mã hóa DES**: Thuật toán DES 16 vòng, khóa 56-bit hiệu dụng, khối 64-bit
- **Truyền qua TCP/IP**: Hỗ trợ truyền file an toàn qua mạng LAN
- **GUI hiện đại**: Giao diện PyQt6 với drag-and-drop file
- **Tabbed Interface**: Tách riêng chức năng Gửi và Nhận file
- **Cross-platform**: Hỗ trợ Windows, Linux, macOS

## 📁 Cấu trúc dự án

```
csat&bmtt/
├── README.md                           # File này
├── GUI/                                # Ứng dụng GUI Python
│   ├── run_app.vbs                   # Launcher (ẩn terminal)
│   ├── app_pro.py                    # Ứng dụng chính (PyQt6)
│   ├── requirements.txt              # Dependencies Python
│   ├── GUI_USAGE.md                  # Hướng dẫn sử dụng GUI
│   └── venv/                         # Virtual environment
│
└── SecureFileTransfer/               # Core DES + Network
    ├── README.md                     # Tài liệu C++
    ├── FILES.md                      # Danh sách file source
    ├── ARCHITECTURE.md               # Kiến trúc hệ thống
    ├── Makefile                      # Build system
    ├── build-mingw.bat               # Build script (Windows)
    ├── bin/                          # Executable files
    │   ├── client_send.exe
    │   └── server_recv.exe
    ├── src/                          # Source code
    │   ├── des/                      # DES algorithm
    │   │   ├── des_core.cpp
    │   │   ├── des_tables.cpp
    │   │   └── des_utils.cpp
    │   └── network/                  # Networking
    │       └── socket_utils.cpp
    ├── include/                      # Header files
    │   ├── des.h
    │   ├── network.h
    │   └── platform.h
    ├── client_main.cpp               # Client application
    ├── server_main.cpp               # Server application
    └── data/                         # Test files
```

## 🔧 Yêu cầu hệ thống

### Để chạy GUI

- Windows 10+ hoặc Linux/macOS
- Python 3.10+
- PyQt6 (sẽ được install tự động)

### Để build từ source (C++)

- GCC 12+ hoặc MinGW (Windows)
- GNU Make 4.0+
- C++17 compiler

## 📥 Cài đặt

### 1. Clone/Download dự án

```bash
cd csat&bmtt
```

### 2. Chạy GUI (Windows)

Double-click `GUI/run_app.vbs` hoặc chạy lệnh:

```bash
cd GUI
python app_pro.py
```

### 3. Build C++ (tùy chọn, nếu cần rebuild)

```bash
cd SecureFileTransfer
make
```

## 🚀 Hướng dẫn sử dụng

### Gửi File

1. Mở `GUI/run_app.vbs`
2. Chọn tab **"📤 Gửi File"**
3. Nhập IP Server (máy nhận)
4. Nhập Port (ví dụ: `5000`)
5. Nhập Key (mật khẩu, ≥8 ký tự)
6. Kéo file vào ô hoặc click để chọn
7. Nhấn **"▶️ GỬI FILE"**

### Nhận File

1. Mở `GUI/run_app.vbs`
2. Chọn tab **"📥 Nhận File"**
3. Nhập Port để lắng nghe (phải giống port gửi)
4. Nhập Key (phải giống key gửi)
5. Chọn thư mục lưu file
6. Nhập tên file (ví dụ: `output.txt`)
7. Nhấn **"▶️ BẮT ĐẦU LẮNG NGHE"**
8. Đợi máy bên gửi kết nối

### Truyền qua Internet (Port Forwarding)

Nếu 2 máy khác mạng:

1. Trên Router máy nhận, forward port (ví dụ 5000) → IP máy nhận
2. Trên GUI gửi, nhập IP Public của máy nhận
3. Phần còn lại như bình thường

## 🏗️ Kiến trúc hệ thống

### DES Encryption

- **Feistel Network**: 16 vòng mã hóa
- **Key Schedule**: Sinh 16 round key từ 56-bit key
- **S-boxes**: Bảng thay thế FIPS 46-3
- **Padding**: PKCS#7 (1-8 bytes)

### Network Protocol

```
[8 bytes: Size Header] [N bytes: Encrypted Data]
```

- Header: Big-endian uint64 (kích thước data)
- Data: Encrypted file bytes

### GUI Architecture

- **DropZone**: Custom QFrame với drag-and-drop
- **TransferThread**: QThread cho async file transfer
- **Tabbed Interface**: QTabWidget (Send/Receive)
- **Rich Logging**: Timestamped log messages

## 📊 File Size

- Executable trung bình: ~200KB (client_send.exe, server_recv.exe)
- Python app: ~15KB (app_pro.py)
- Virtual environment: ~150MB (PyQt6 + dependencies)

## 🔒 Security

- **DES Algorithm**: FIPS 46-3 compliant
- **Key Derivation**: From ASCII password
- **Padding**: PKCS#7 validated
- **TCP**: Unencrypted network layer (optional TLS upgrade)

## 📝 License

Dự án học tập - CSAT & BMTT

## 👨‍💻 Tác giả

Sinh viên Nam 4 - Học kỳ II

---

**Ghi chú**: Để biết chi tiết hơn, xem:

- `GUI/GUI_USAGE.md` - Hướng dẫn GUI chi tiết
- `SecureFileTransfer/ARCHITECTURE.md` - Kiến trúc C++
- `SecureFileTransfer/README.md` - Build instructions
