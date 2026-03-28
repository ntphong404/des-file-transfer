# Hướng Dẫn Chạy (Windows MinGW + Make)

## ⚡ Cách Nhanh Nhất (3 Lệnh)

### 1️⃣ Biên Dịch

```bash
cd D:\Phong\Mon_dai_hoc\Nam_4\ki2\csat&bmtt\SecureFileTransfer
make clean
make all
```

### 2️⃣ Chạy Server (Terminal 1)

```bash
bin\server_recv.exe 5000 output.txt 12345678
```

### 3️⃣ Chạy Client (Terminal 2)

```bash
bin\client_send.exe 127.0.0.1 5000 data\plain.txt 12345678
```

### 4️⃣ Kiểm Tra Kết Quả

```bash
(Get-FileHash data\plain.txt).Hash -eq (Get-FileHash output.txt).Hash
```

✅ **Nếu kết quả True → Thành công!**

---

## 📋 Các Lệnh Make

```bash
make all            # Biên dịch tất cả (MẶC ĐỊNH)
make clean          # Xóa bin/ + obj/
make clean-obj      # Xóa chỉ obj/
make run-server     # Chạy server (mặc định: 5000, password: 12345678)
make run-client     # Chạy client (mặc định)
make info           # Hiển thị compiler info
make help           # Hiển thị tất cả targets
```

---

## 🔧 Tùy Chỉnh

### Thay Đổi Port

```bash
# Server
bin\server_recv.exe 9000 output.txt mykey123

# Client (match với server)
bin\client_send.exe 127.0.0.1 9000 data\plain.txt mykey123
```

### Thay Đổi Khóa DES

```bash
# Khóa phải đúng 8 ký tự, giống nhau trên client & server
bin\server_recv.exe 5000 output.txt mykey123
bin\client_send.exe 127.0.0.1 5000 data\plain.txt mykey123
```

### Mã Hóa File Khác

```bash
# Gửi file khác
bin\client_send.exe 127.0.0.1 5000 data\image.jpg mykey123

# Output file khác
bin\server_recv.exe 5000 result.bin mykey123
```

---

## ✅ Kiểm Tra Công Cụ

Trước khi biên dịch, đảm bảo đã cài:

```bash
gcc --version          # GCC 5.0+ required
g++ --version          # G++ 5.0+ required
make --version         # Make 3.8+ required
```

Nếu thiếu:

```bash
choco install mingw make -y
```

---

## 📊 Output Ví Dụ

### Compile

```
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude -c src/des/des_tables.cpp -o obj/src/des/des_tables.o
...
ar rcs obj/libdes.a ...
ar rcs obj/libnetwork.a ...
g++ ... -o bin/client_send.exe
g++ ... -o bin/server_recv.exe
```

### Server Running

```
=== DES File Reception & Decryption (Server) ===
Listening on port: 5000
Output file: output.txt
Key: 12345678

[1] Creating server socket...
    Server listening on port 5000

[2] Waiting for client connection...
```

### Client Running

```
=== DES File Encryption & Transmission (Client) ===
Server: 127.0.0.1:5000
Input file: data/plain.txt
Key: 12345678

[1] Reading input file...
    Plaintext size: 856 bytes

[4] Encrypting file (ECB mode)...
    Ciphertext size: 864 bytes

[5] Connecting to server...
    Connected to server 127.0.0.1:5000
...
=== Transmission Complete ===
```

---

## ❌ Lỗi Thường Gặp

| Lỗi                       | Nguyên Nhân              | Cách Sửa                        |
| ------------------------- | ------------------------ | ------------------------------- |
| `make: command not found` | Make chưa cài            | `choco install make`            |
| `g++: command not found`  | MinGW chưa cài           | `choco install mingw`           |
| `Address already in use`  | Port 5000 bị chiếm       | Dùng port khác: `-p 5001`       |
| `Cannot read input file`  | File input không tồn tại | Kiểm tra path: `data\plain.txt` |
| `Connection refused`      | Server chưa chạy         | Chạy server trước               |
| `Invalid key length`      | Khóa không 8 ký tự       | Khóa phải đúng 8 ký tự          |
| `.exe not found`          | Chưa biên dịch           | Chạy `make all` trước           |

---

## 🛠️ Build Options

### Debug Build (có symbols)

Sửa Makefile:

```makefile
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0
```

Sau đó:

```bash
make clean
make all
```

### Release Build Tối Ưu

Sửa Makefile:

```makefile
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native
```

---

## 📝 Workflow Hoàn Chỉnh

```
1. Build
   $ make clean
   $ make all
   ✓ bin\client_send.exe created
   ✓ bin\server_recv.exe created

2. Terminal 1 - Start Server
   $ bin\server_recv.exe 5000 output.txt 12345678
   ✓ Server listening on port 5000...

3. Terminal 2 - Start Client
   $ bin\client_send.exe 127.0.0.1 5000 data\plain.txt 12345678
   ✓ Connected to server
   ✓ Data sent

4. Verify
   $ fc data\plain.txt output.txt
   ✓ Files match!

5. Cleanup (optional)
   $ make clean
```

---

## 🔄 Test với File Khác Nhau

### Test 1: Same Machine, Different Ports

```bash
# Terminal 1
bin\server_recv.exe 5001 output1.txt key12345

# Terminal 2
bin\client_send.exe 127.0.0.1 5001 data\plain.txt key12345
```

### Test 2: Different Files

```bash
# Tạo file test
echo "Hello World!" > data\test.txt

# Terminal 1
bin\server_recv.exe 5002 output2.txt pwdpwdpw

# Terminal 2
bin\client_send.exe 127.0.0.1 5002 data\test.txt pwdpwdpw
```

### Test 3: Network (Different Machines)

```bash
# Server machine (192.168.1.100)
bin\server_recv.exe 5000 output.txt secret123

# Client machine
bin\client_send.exe 192.168.1.100 5000 data\plain.txt secret123
```

---

## ⏱️ Thời Gian Biên Dịch

```
First build:    ~3-5 seconds (toàn bộ)
Incremental:    <1 second (chỉ file thay đổi)
After clean:    ~2-3 seconds
```

---

## 📚 Tài Liệu Thêm

- **Kiến trúc**: [ARCHITECTURE.md](ARCHITECTURE.md)
- **Danh sách file**: [FILES.md](FILES.md)
- **Tổng quan**: [README.md](README.md)

---

**Created**: March 28, 2026  
**For**: Windows MinGW 15.2.0 + GNU Make 4.4.1  
**Status**: ✅ Ready to use
