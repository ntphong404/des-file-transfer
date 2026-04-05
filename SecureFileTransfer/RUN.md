# Hướng Dẫn Build & Chạy

## ⚡ Cách Nhanh Nhất

### 1️⃣ Build

```bash
cd SecureFileTransfer
make clean
make all
# → bin/client_send.exe, bin/server_recv.exe
```

### 2️⃣ Chạy Server (Terminal 1 — máy nhận)

```bash
bin\server_recv.exe <port> <output_dir> <key>

# Ví dụ:
bin\server_recv.exe 5000 D:\received 12345678
```

### 3️⃣ Chạy Client (Terminal 2 — máy gửi)

```bash
bin\client_send.exe <ip> <port> <key> <file1> [file2] [file3]...

# Ví dụ gửi 1 file:
bin\client_send.exe 127.0.0.1 5000 12345678 data\plain.txt

# Ví dụ gửi nhiều file:
bin\client_send.exe 127.0.0.1 5000 12345678 report.pdf photo.jpg notes.txt
```

> **Lưu ý**: Dùng Key >= 24 ký tự để tự động bật chế độ **3DES** (Triple DES). Dưới 24 ký tự sẽ dùng **DES** (8 byte đầu). Key giống nhau cả 2 đầu mới giải mã được.

### 4️⃣ Kiểm Tra Kết Quả

```powershell
# PowerShell
(Get-FileHash data\plain.txt).Hash -eq (Get-FileHash D:\received\plain.txt).Hash
# → True = thành công
```

---

## 🖥️ Chạy Qua GUI (khuyến nghị)

```bash
cd GUI
venv\Scripts\activate
python app_pro.py
```

Hoặc double-click `GUI/run_app.vbs`.

---

## 📋 Các Lệnh Make

```bash
make all            # Build tất cả (mặc định)
make clean          # Xóa bin/ + obj/
make clean-obj      # Xóa chỉ obj/
make run-server     # Chạy server mặc định (port 5000, key 12345678)
make run-client     # Chạy client mặc định
make info           # Hiển thị compiler info
make help           # Hiển thị tất cả targets
```

---

## 🔧 Tuỳ Chỉnh

### Thay Đổi Port

```bash
bin\server_recv.exe 9000 D:\received mykey123
bin\client_send.exe 192.168.1.5 9000 mykey123 file.txt
```

### Gửi Nhiều File Cùng Lúc

```bash
bin\client_send.exe 127.0.0.1 5000 12345678 file1.pdf file2.jpg file3.docx
```

Server tự nhận và lưu từng file với tên gốc vào `output_dir`.

### Gửi và Nhận Bằng 3DES (Chế Độ Bảo Mật Cao)

Cơ chế: Nếu Key bạn truyền vào độ dài **>= 24 ký tự**, hệ thống C++ sẽ tự động nhận diện và sử dụng thuật toán **3DES** thay vì DES.

```bash
# Server lắng nghe bằng 3DES
bin\server_recv.exe 5000 D:\received 1234567887654321abcdefgh

# Client gửi bằng 3DES
bin\client_send.exe 127.0.0.1 5000 1234567887654321abcdefgh important.pdf
```

### Tên File Tiếng Việt

Hoàn toàn hỗ trợ — cả client và server đều dùng UTF-8 qua `GetCommandLineW()` / `_wfopen`.

```bash
bin\client_send.exe 127.0.0.1 5000 12345678 "Bài tập nhóm.pdf" "Hình ảnh.png"
```

---

## ✅ Kiểm Tra Công Cụ

```bash
gcc --version       # GCC 10+ required
g++ --version
make --version      # Make 4.0+ required
```

Nếu thiếu (Windows):

```bash
choco install mingw make -y
```

---

## 📊 Output Ví Dụ

### Server

```
=== DES Multi-File Reception & Decryption (Server) ===
Port      : 5000
Output dir: D:\received

[1] Creating server socket...
[2] Waiting for client connection...
[3] Expecting 3 file(s)...

--- File [1/3] ---
  Filename  : report.pdf
  Data size : 204800 bytes (encrypted)
  Decrypted : 204793 bytes
  Saved to  : D:\received/report.pdf  [OK]

--- File [2/3] ---
  ...

=== Result: 3/3 files received successfully ===
```

### Client

```
=== 3DES Multi-File Encryption & Transmission (Client) ===
Server : 127.0.0.1:5000
Mode   : 3DES (24-byte key) EDE3
Files  : 3

[1/3] Reading & encrypting: report.pdf
      204793 bytes → 204800 bytes (encrypted)
[2/3] Reading & encrypting: photo.jpg
      ...
[4] Connecting to 127.0.0.1:5000...
[5] Sending 3 file(s)...
  [1/3] report.pdf → sent 204800 bytes  [OK]
  [2/3] photo.jpg  → sent ...           [OK]
  ...
=== All 3 files sent successfully ===
```

---

## ❌ Lỗi Thường Gặp

| Lỗi | Nguyên Nhân | Cách Sửa |
|-----|-------------|----------|
| `make: command not found` | Make chưa cài | `choco install make` |
| `g++: command not found` | MinGW chưa cài | `choco install mingw` |
| `Connection refused` | Server chưa chạy | Chạy server trước client |
| `Address already in use` | Port đang bị chiếm | Dùng port khác |
| `Cannot open file` | File không tồn tại | Kiểm tra đường dẫn |
| `Invalid padding` | Chết Padding do lệch Key / lệch thuật toán (DES vs 3DES) | Đảm bảo Key và độ dài Key (chuẩn DES/3DES) giống nhau |
| `.exe not found` | Chưa build | Chạy `make all` trước |

---

## 🔄 Kịch Bản Test

### Test 1: Cùng máy — nhiều file

```bash
bin\server_recv.exe 5000 .\received 12345678
bin\client_send.exe 127.0.0.1 5000 12345678 data\plain.txt data\image.jpg
```

### Test 2: Khác máy trong LAN

```bash
# Máy A (nhận) — IP: 192.168.1.10
bin\server_recv.exe 5000 D:\received secret1234

# Máy B (gửi)
bin\client_send.exe 192.168.1.10 5000 secret1234 "Báo cáo.docx" "Slide.pptx"
```

### Test 3: Kiểm tra tính toàn vẹn

```powershell
# So sánh hash file gốc vs file nhận
$original = (Get-FileHash "data\plain.txt").Hash
$received = (Get-FileHash ".\received\plain.txt").Hash
if ($original -eq $received) { "✅ PASS" } else { "❌ FAIL" }
```

---

**Created**: March 2026 | **Updated**: April 2026 (Added 3DES)
**Platform**: Windows MinGW 15.2.0 + GNU Make 4.4.1
**Status**: ✅ Ready to use (DES + 3DES Supported)
