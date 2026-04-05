# 🔐 DES File Transfer Pro

Ứng dụng mã hóa và truyền **nhiều file** an toàn qua mạng LAN sử dụng thuật toán DES thuần C++.

## 📋 Mục lục

- [Tính năng](#tính-năng)
- [Cấu trúc dự án](#cấu-trúc-dự-án)
- [Yêu cầu hệ thống](#yêu-cầu-hệ-thống)
- [Cài đặt & Chạy](#cài-đặt--chạy)
- [Hướng dẫn sử dụng](#hướng-dẫn-sử-dụng)
- [Kiến trúc hệ thống](#kiến-trúc-hệ-thống)

## ✨ Tính năng

- **Mã hóa DES / 3DES**: Hỗ trợ thuật toán DES thuần (8-byte key) và chế độ bảo mật cao 3DES EDE3 (Triple DES, 24-byte key)
- **Gửi nhiều file một lúc**: Protocol mới hỗ trợ batch transfer N file trong 1 kết nối
- **Tên file tự động**: Server tự lưu đúng tên file gốc (kể cả tên tiếng Việt)
- **GUI hiện đại**: Giao diện PyQt6 với drag-and-drop nhiều file, danh sách file nhận real-time
- **Scroll toàn app**: Không bị tràn màn hình, hỗ trợ mọi độ phân giải
- **Cross-platform**: Windows (MinGW), Linux, macOS
- **Hỗ trợ tiếng Việt**: Tên file UTF-8 hoạt động đúng trên Windows

## 📁 Cấu trúc dự án

```
csat&bmtt/
├── README.md                           # File này
├── GUI/                                # Ứng dụng GUI Python
│   ├── run_app.vbs                   # Launcher (ẩn terminal)
│   ├── app_pro.py                    # Ứng dụng chính (PyQt6)
│   ├── requirements.txt              # Dependencies Python
│   └── venv/                         # Virtual environment
│
└── SecureFileTransfer/               # Core DES + Network (C++)
    ├── ARCHITECTURE.md               # Kiến trúc & protocol chi tiết
    ├── FILES.md                      # Danh sách file source
    ├── RUN.md                        # Hướng dẫn build & chạy CLI
    ├── Makefile                      # Build system (GNU Make)
    ├── build-mingw.bat               # Build script (Windows)
    ├── bin/                          # Executable files
    │   ├── client_send.exe           # Chạy trên máy gửi
    │   └── server_recv.exe           # Chạy trên máy nhận
    ├── src/                          # Source code
    │   ├── des/
    │   │   ├── des_core.cpp          # Thuật toán DES (Feistel 16 vòng)
    │   │   ├── des_tables.cpp        # Bảng DES (FIPS 46-3)
    │   │   └── des_utils.cpp         # File I/O + padding + UTF-8
    │   └── network/
    │       └── socket_utils.cpp      # TCP socket cross-platform
    ├── include/                      # Header files
    │   ├── des/
    │   └── network/
    ├── client_main.cpp               # Client application
    ├── server_main.cpp               # Server application
    └── data/                         # Test files
```

## 🔧 Yêu cầu hệ thống

### Để chạy GUI

- Windows 10+ (hoặc Linux/macOS)
- Python 3.10+
- PyQt6 (cài qua pip)

### Để build từ source (C++)

- GCC/G++ 10+ hoặc MinGW-w64 (Windows)
- GNU Make 4.0+
- Không cần thư viện ngoài (Winsock2 built-in trên Windows)

## 📥 Cài đặt & Chạy

### 1. Chạy GUI (cách nhanh nhất)

```bash
cd GUI
# Kích hoạt virtual environment
venv\Scripts\activate          # Windows
source venv/bin/activate       # Linux/macOS

python app_pro.py
```

Hoặc double-click `GUI/run_app.vbs` trên Windows.

### 2. Build lại C++ (nếu cần)

```bash
cd SecureFileTransfer
make clean
make all
# → tạo bin/client_send.exe, bin/server_recv.exe
```

### 3. Cài PyQt6 (lần đầu)

```bash
cd GUI
python -m venv venv
venv\Scripts\activate
pip install PyQt6
```

## 🚀 Hướng dẫn sử dụng

### Tab Gửi File

1. Mở app → chọn tab **"📤 Gửi File"**
2. Nhập **IP Server** (máy nhận), **Port** (ví dụ: `5000`), **Key** (≥8 ký tự). Có thể tích chọn **Dùng thuật toán 3DES** (tự động mở thêm 2 trường Key).
3. Kéo thả nhiều file vào Drop Zone **hoặc** nhấn "Chọn File..."
4. Xem danh sách file muốn gửi (có thể xoá từng file)
5. Nhấn **"▶ GỬI FILE"**

### Tab Nhận File

1. Chọn tab **"📥 Nhận File"**
2. Nhập **Port** lắng nghe (phải giống bên gửi), **Key** (phải giống bên gửi)
3. Chọn **thư mục lưu file** (tạo tự động nếu chưa có)
4. Nhấn **"▶ BẮT ĐẦU LẮNG NGHE"**
5. Đợi bên gửi kết nối → file sẽ tự lưu với **tên gốc** vào thư mục đã chọn
6. Danh sách file nhận được hiển thị real-time trong panel "Kết Quả Nhận"

### Chạy qua CLI (không dùng GUI)

```bash
# Máy nhận (chạy trước)
bin\server_recv.exe <port> <output_dir> <key>
bin\server_recv.exe 5000 D:\received 12345678

# Máy gửi
bin\client_send.exe <ip> <port> <key> <file1> [file2] [file3]...
bin\client_send.exe 192.168.1.100 5000 12345678 report.pdf photo.jpg

# Sử dụng 3DES (Chỉ cần truyền key >24 ký tự)
bin\client_send.exe 127.0.0.1 5000 1234567887654321abcdefgh report.pdf
```

## 🏗️ Kiến trúc hệ thống

### Network Protocol (Multi-File)

```
[4B: num_files]
Với mỗi file:
  [4B: filename_len] [filename_len bytes: tên file UTF-8]
  [8B: encrypted_data_size] [encrypted_data bytes]
```

### DES Encryption

- **Mode**: ECB (Electronic Codebook)
- **Feistel Network**: DES chuẩn 16 vòng. Với 3DES: 48 vòng (Encrypt -> Decrypt -> Encrypt).
- **Key Schedule**: Sinh 16 round key từ 1 key 8-byte (DES), sinh 48 round key từ cụm 24-byte key (3DES).
- **S-boxes**: Bảng thay thế FIPS 46-3
- **Padding**: PKCS#7 (1–8 bytes)

### GUI Architecture

```
QMainWindow
├── Header (cố định)
└── QScrollArea (toàn bộ body có thể cuộn)
     ├── QTabWidget
     │    ├── Tab Gửi: DropZone + FileList + SendBtn
     │    └── Tab Nhận: Config + OutputDir + ReceivedList + RecvBtn
     └── Log Panel (dark theme, real-time)
```

- **TransferThread**: QThread chạy subprocess C++ async, có `kill()` khi đóng app
- **MultiFileDropZone**: QFrame nhận drag-and-drop nhiều file
- **FileListWidget**: Danh sách file với kích thước, xoá từng file

## 🔒 Bảo mật

| Thành phần | Chi tiết |
|---|---|
| Thuật toán | DES / 3DES (Triple DES - EDE3) |
| Mode | ECB — phù hợp học tập |
| Key | 8 bytes (DES) hoặc 24 bytes (3DES) |
| Padding | PKCS#7 được validate khi giải mã |
| Transport | TCP plaintext (không có TLS) |

> ⚠️ DES ECB mode không an toàn cho môi trường production. Dự án này phục vụ mục đích học tập về CSAT & BMTT.

## 📝 License

Dự án học tập — CSAT & BMTT — Năm 4, Học kỳ II

---

Xem thêm: [`SecureFileTransfer/ARCHITECTURE.md`](SecureFileTransfer/ARCHITECTURE.md) · [`SecureFileTransfer/RUN.md`](SecureFileTransfer/RUN.md)
