# 🔐 DES File Transfer GUI - Hướng Dẫn Sử Dụng

## 🚀 Cách Chạy

### **Cách 1: Chạy file VBS (Dễ nhất - ẩn terminal)**

```
Double-click: run_app.vbs
```

File này sẽ chạy app Python hoàn toàn ẩn terminal.

### **Cách 2: Chạy thủ công từ terminal (debug)**

```powershell
cd 'D:\Phong\Mon_dai_hoc\Nam_4\ki2\csat&bmtt\GUI'
.\venv\Scripts\python app_pro.py
```

---

## 📖 Hướng Dẫn Sử Dụng

### **Giao Diện Chính** - PyQt6 Tabbed Interface

Ứng dụng có 2 tabs chính với header xanh đẹp:

```
┌────────────────────────────────────────────────────┐
│  🔐 DES FILE TRANSFER PRO                          │
│  Gửi/Nhận file mã hóa an toàn qua mạng LAN         │
├────────────────────────────────────────────────────┤
│   [📤 Gửi File]  [📥 Nhận File]                   │
├────────────────────────────────────────────────────┤
│  ⚙️  Cấu Hình                                      │
│  [Settings fields...]                              │
│                                                    │
│  📁 Chọn File (Drag & Drop)                        │
│  [Kéo file vào đây hoặc click để chọn]             │
│                                                    │
│  [▶️  GỬI FILE]                                    │
├────────────────────────────────────────────────────┤
│  📋 Nhật Ký Hoạt Động                              │
│  [Log messages with timestamps...]                 │
└────────────────────────────────────────────────────┘
```

---

## 📤 Tab: Gửi File (Client)

### **Bước 1: Nhập Cấu Hình**

- **IP Server**: IP của máy nhận (ví dụ: `192.168.1.100` hoặc `127.0.0.1` nếu cùng máy)
- **Port**: Cổng mà server đang lắng nghe (mặc định: `5000`)
- **Key (Mật khẩu)**: Mật khẩu để mã hóa (ít nhất 8 ký tự, phải khớp với server)

### **Bước 2: Chọn File Để Gửi**

**Cách 1: Kéo file vào**

- Kéo file từ File Explorer vào ô "Kéo file vào đây"
- Ô sẽ đổi sang xanh nhạt (#e3f2fd) khi kéo vào

**Cách 2: Click để chọn**

- Click vào text màu xanh "nhấp để chọn file"
- Mở dialog chọn file

**Thông tin file được hiển thị:**

- Tên file
- Kích thước (MB)
- Thời gian hiện tại

### **Bước 3: Gửi File**

- Click nút **"▶️ GỬI FILE"**
- Theo dõi nhật ký hoạt động để xem kết quả

### **Kết quả Thành Công**

```
[12:34:56] [INFO] ℹ️  File: document.pdf
[12:34:56] [INFO] ℹ️  Kích thước: 2.50 MB
[12:34:57] [INFO] ℹ️  Gửi: document.pdf
[12:34:58] [SUCCESS] ✅ Thành công!
```

---

## 📥 Tab: Nhận File (Server)

### **Bước 1: Nhập Cấu Hình Server**

- **Port Lắng Nghe**: Port để server lắng nghe (mặc định: `5000`, **phải khớp với port client**)
- **Key (Mật khẩu)**: Mật khẩu để giải mã (**phải giống key ở client**)

### **Bước 2: Chọn Nơi Lưu File**

- Click **"📂 Chọn Thư Mục"** để lựa chọn thư mục đích

### **Bước 3: Nhập Tên File**

- **Tên File**: Nhập tên file output (ví dụ: `output.txt`, `received.mp4`)
- Mặc định: `output.txt`

### **Bước 4: Bắt Đầu Lắng Nghe**

- Click nút **"▶️ BẮT ĐẦU LẮNG NGHE"**
- Server sẽ chờ client kết nối

### **Kết quả Thành Công**

```
[12:34:56] [INFO] ℹ️  Lắng nghe trên port 5000...
[12:34:58] [INFO] ℹ️  Client kết nối
[12:34:59] [INFO] ℹ️  Giải mã và lưu file...
[12:35:00] [SUCCESS] ✅ Hoàn thành!
```

---

## 🔗 Ví Dụ Thực Tế - Cùng Máy (Localhost)

### **Cửa sổ 1 - Tab "📥 Nhận File" (Server)**

1. Double-click `run_app.vbs`
2. Chọn Tab **"📥 Nhận File"**
3. Port: `5000`, Key: `12345678`
4. Thư mục: `C:\Downloads`, Tên: `output.txt`
5. Click **"▶️ BẮT ĐẦU LẮNG NGHE"**
6. Chờ...

### **Cửa sổ 2 - Tab "📤 Gửi File" (Client)**

1. Double-click `run_app.vbs` (cửa sổ mới)
2. Chọn Tab **"📤 Gửi File"** (mặc định)
3. IP: `127.0.0.1`, Port: `5000`, Key: `12345678`
4. Kéo file vào hoặc click để chọn
5. Click **"▶️ GỬI FILE"**

**Kết quả:** File được mã hóa → gửi → giải mã → lưu vào `output.txt`

---

## 🌐 Ví Dụ Thực Tế - Khác Máy (LAN)

### **Máy A (Server - Nhận File)**

1. Chạy `run_app.vbs`
2. Chọn Tab **"📥 Nhận File"**
3. Port: `5000`, Key: `mykey123`
4. Thư mục: `C:\Received`
5. Tên File: `received.mp4`
6. Click **"▶️ BẮT ĐẦU LẮNG NGHE"**

### **Máy B (Client - Gửi File)**

1. Chạy `run_app.vbs`
2. Chọn Tab **"📤 Gửi File"**
3. **IP Server**: `192.168.1.50` ← IP của Máy A
4. **Port**: `5000` ← Phải giống server
5. **Key**: `mykey123` ← ⚠️ **PHẢI KHỚP!**
6. Kéo file hoặc chọn
7. Click **"▶️ GỬI FILE"**

### **Yêu cầu:**

- ✅ Cả 2 máy kết nối cùng WiFi/Ethernet
- ✅ Port không bị firewall chặn
- ✅ Key giống nhau (case-sensitive)

---

## 🆘 Troubleshooting

### **Lỗi: "Không tìm thấy bin/client_send.exe"**

- **Nguyên nhân**: Chưa compile C++
- **Cách sửa**:
  ```powershell
  cd SecureFileTransfer
  make all
  ```

### **Lỗi: "Port phải từ 1-65535"**

- **Nguyên nhân**: Port nhập sai
- **Cách sửa**: Nhập port từ 1-65535 (mặc định: 5000)

### **Lỗi: "Key phải ≥ 8 ký tự"**

- **Nguyên nhân**: Key quá ngắn
- **Cách sửa**: Nhập key 8+ ký tự

### **Lỗi: "Connection refused" / Timeout**

- **Nguyên nhân**: Server chưa start hoặc IP/Port sai
- **Cách sửa**:
  1. Chắc chắn server đang chạy
  2. Kiểm tra IP: `ipconfig` (tìm IPv4)
  3. Kiểm tra port khớp

### **File gửi không tồn tại**

- **Nguyên nhân**: Đường dẫn sai hoặc file đã xóa
- **Cách sửa**: Click vào text để chọn file tồn tại

---

## 📋 Nhật Ký Hoạt Động

- Hiển thị tất cả sự kiện với **timestamp** (HH:MM:SS)
- **Icons**: ℹ️ (info), ✅ (success), ❌ (error), ⚠️ (warning)
- Scroll tự động đến dòng cuối

---

## 📊 Thông Tin Kỹ Thuật

### **DES Encryption**

- Thuật toán: 16-round Feistel
- Khóa: 56-bit hiệu dụng (từ 8 ký tự ASCII)
- Block: 64-bit
- Padding: PKCS#7

### **Network Protocol**

- Giao thức: TCP/IP
- Header: 8 bytes (size big-endian)
- Payload: Encrypted data

### **GUI Framework**

- Python: 3.10+
- Framework: PyQt6 6.0+
- Platform: Windows, Linux, macOS

---

⚠️ **Lưu ý Bảo Mật:**  
DES chỉ dùng cho học tập. 56-bit key có thể bị brute-force.  
Để bảo mật thực tế, dùng AES-256.

---

**Phiên bản**: 1.0 (2026-03-29) - PyQt6 Professional  
**Status**: Production Ready ✅
