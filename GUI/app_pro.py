#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DES File Transfer - Professional PyQt6 GUI with Tabs
Modern interface with drag & drop, file preview, and tabbed layout
"""

import sys
from pathlib import Path
import subprocess
import threading
import os
from datetime import datetime

from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                             QLabel, QLineEdit, QPushButton, QTabWidget, QTextEdit, 
                             QFileDialog, QGroupBox, QFormLayout, QFrame, QProgressBar)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize, QMimeData, QUrl
from PyQt6.QtGui import QFont, QDrag, QPixmap, QColor, QTextCursor, QIcon, QCursor
from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout


class DropZone(QFrame):
    """Khu vực kéo thả file - kiểu Postman đơn giản"""
    file_dropped = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.setStyleSheet("""
            QFrame {
                background-color: transparent;
                border: none;
            }
        """)
        self.setAcceptDrops(True)
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(4)
        
        title = QLabel("Kéo file vào đây")
        title.setFont(QFont("Arial", 12, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        or_text = QLabel("hoặc")
        or_text.setFont(QFont("Arial", 9))
        or_text.setAlignment(Qt.AlignmentFlag.AlignCenter)
        or_text.setStyleSheet("color: #999;")
        
        click_text = QLabel('<a href="#" style="color: #0078d4; text-decoration: underline;">nhấp để chọn file</a>')
        click_text.setFont(QFont("Arial", 10, QFont.Weight.Bold))
        click_text.setAlignment(Qt.AlignmentFlag.AlignCenter)
        click_text.setOpenExternalLinks(False)
        click_text.linkActivated.connect(self.on_link_clicked)
        
        layout.addWidget(title)
        layout.addWidget(or_text)
        layout.addWidget(click_text)
        
        self.file_path = None
    
    def on_link_clicked(self):
        """Xử lý click vào link"""
        file_path, _ = QFileDialog.getOpenFileName()
        if file_path:
            self.file_path = file_path
            self.file_dropped.emit(self.file_path)
    
    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            self.setStyleSheet("""
                QFrame {
                    background-color: #e3f2fd;
                    border: none;
                }
            """)
    
    def dragLeaveEvent(self, event):
        self.setStyleSheet("""
            QFrame {
                background-color: transparent;
                border: none;
            }
        """)
    
    def dropEvent(self, event):
        files = [url.toLocalFile() for url in event.mimeData().urls()]
        if files:
            self.file_path = files[0]
            self.file_dropped.emit(self.file_path)
        
        self.setStyleSheet("""
            QFrame {
                background-color: transparent;
                border: none;
            }
        """)


class TransferThread(QThread):
    """Thread để thực hiện transfer"""
    log_signal = pyqtSignal(str, str)
    progress_signal = pyqtSignal(int)
    finished_signal = pyqtSignal(bool)
    
    def __init__(self, cmd, cwd):
        super().__init__()
        self.cmd = cmd
        self.cwd = cwd
    
    def run(self):
        try:
            proc = subprocess.Popen(
                self.cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                cwd=self.cwd
            )
            
            for line in proc.stdout:
                line = line.rstrip()
                if line:
                    self.log_signal.emit(line, "INFO")
            
            proc.wait()
            
            if proc.returncode == 0:
                self.log_signal.emit("✅ Thành công!", "SUCCESS")
                self.finished_signal.emit(True)
            else:
                self.log_signal.emit(f"❌ Lỗi: Exit code {proc.returncode}", "ERROR")
                self.finished_signal.emit(False)
        except Exception as e:
            self.log_signal.emit(f"❌ Lỗi: {e}", "ERROR")
            self.finished_signal.emit(False)


class DESTransferApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("🔐 DES File Transfer Pro")
        self.setGeometry(100, 100, 800, 850)
        self.setMinimumWidth(750)
        
        # Paths
        self.app_dir = Path(__file__).parent
        self.project_dir = self.app_dir.parent / "SecureFileTransfer"
        self.bin_dir = self.project_dir / "bin"
        
        self.transfer_thread = None
        self.selected_file = None
        
        self.init_ui()
        self.apply_styles()
    
    def init_ui(self):
        """Initialize UI"""
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        main_layout.setContentsMargins(8, 0, 8, 8)
        main_layout.setSpacing(8)
        
        # === Header ===
        header = QWidget()
        header.setStyleSheet("background-color: #0078d4; padding: 12px 15px;")
        header_layout = QVBoxLayout(header)
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.setSpacing(4)
        
        title = QLabel("🔐 DES FILE TRANSFER PRO")
        title.setFont(QFont("Arial", 18, QFont.Weight.Bold))
        title.setStyleSheet("color: white;")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        subtitle = QLabel("Gửi/Nhận file mã hóa an toàn qua mạng LAN")
        subtitle.setFont(QFont("Arial", 9))
        subtitle.setStyleSheet("color: #e0e0e0;")
        subtitle.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        header_layout.addWidget(title)
        header_layout.addWidget(subtitle)
        main_layout.addWidget(header)
        
        # === Tabs ===
        tabs = QTabWidget()
        tabs.setFont(QFont("Arial", 11))
        
        # Tab 1: Send
        self.send_tab = self.create_send_tab()
        tabs.addTab(self.send_tab, "📤 Gửi File")
        
        # Tab 2: Receive
        self.recv_tab = self.create_receive_tab()
        tabs.addTab(self.recv_tab, "📥 Nhận File")
        
        main_layout.addWidget(tabs)
        
        # === Log ===
        log_group = QGroupBox("📋 Nhật Ký Hoạt Động")
        log_layout = QVBoxLayout()
        log_layout.setContentsMargins(8, 8, 8, 8)
        log_layout.setSpacing(4)
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setFont(QFont("Courier", 8))
        self.log_text.setMaximumHeight(120)
        log_layout.addWidget(self.log_text)
        log_group.setLayout(log_layout)
        main_layout.addWidget(log_group)
        
        # === Progress ===
        self.progress = QProgressBar()
        self.progress.setVisible(False)
        main_layout.addWidget(self.progress)
    
    def create_send_tab(self):
        """Tab Gửi File"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(10)
        
        # Common settings
        common_group = QGroupBox("⚙️  Cấu Hình Gửi")
        common_layout = QFormLayout()
        common_layout.setVerticalSpacing(8)
        common_layout.setHorizontalSpacing(10)
        
        self.send_ip = QLineEdit("127.0.0.1")
        self.send_port = QLineEdit("5000")
        self.send_key = QLineEdit("12345678")
        self.send_key.setEchoMode(QLineEdit.EchoMode.Password)
        
        common_layout.addRow("IP Server:", self.send_ip)
        common_layout.addRow("Port:", self.send_port)
        common_layout.addRow("Key (Mật khẩu):", self.send_key)
        
        common_group.setLayout(common_layout)
        layout.addWidget(common_group)
        
        # File selection
        file_group = QGroupBox("📁 Chọn File")
        file_layout = QVBoxLayout()
        file_layout.setContentsMargins(10, 15, 10, 15)
        file_layout.setSpacing(0)
        
        # Drop zone
        self.drop_zone = DropZone()
        self.drop_zone.file_dropped.connect(self.on_file_selected)
        self.drop_zone.setMinimumHeight(140)
        file_layout.addWidget(self.drop_zone)
        
        # File info
        self.send_file_info = QLabel("Chưa chọn file")
        self.send_file_info.setStyleSheet("color: #666; font-size: 9px; margin-top: 8px;")
        file_layout.addWidget(self.send_file_info)
        
        file_group.setLayout(file_layout)
        layout.addWidget(file_group)
        
        # Send button
        send_btn = QPushButton("▶️  GỬI FILE")
        send_btn.setFixedHeight(42)
        send_btn.setFont(QFont("Arial", 11, QFont.Weight.Bold))
        send_btn.clicked.connect(self.send_file)
        layout.addWidget(send_btn)
        
        layout.addStretch()
        return widget
    
    def create_receive_tab(self):
        """Tab Nhận File"""
        widget = QWidget()
        layout = QVBoxLayout(widget)
        layout.setContentsMargins(12, 12, 12, 12)
        layout.setSpacing(10)
        
        # Server settings
        server_group = QGroupBox("⚙️  Cấu Hình Server")
        server_layout = QFormLayout()
        server_layout.setVerticalSpacing(8)
        server_layout.setHorizontalSpacing(10)
        
        self.recv_port = QLineEdit("5000")
        self.recv_key = QLineEdit("12345678")
        self.recv_key.setEchoMode(QLineEdit.EchoMode.Password)
        
        server_layout.addRow("Port Lắng Nghe:", self.recv_port)
        server_layout.addRow("Key (Mật khẩu):", self.recv_key)
        
        server_group.setLayout(server_layout)
        layout.addWidget(server_group)
        
        # Output directory
        output_group = QGroupBox("💾 Nơi Lưu File")
        output_layout = QVBoxLayout()
        output_layout.setContentsMargins(10, 10, 10, 10)
        output_layout.setSpacing(8)
        
        output_path_layout = QHBoxLayout()
        output_path_layout.setSpacing(8)
        self.recv_path = QLineEdit()
        self.recv_path.setText(str(self.project_dir))
        browse_path_btn = QPushButton("📂 Chọn Thư Mục")
        browse_path_btn.setMaximumWidth(120)
        browse_path_btn.clicked.connect(self.browse_recv_folder)
        output_path_layout.addWidget(self.recv_path)
        output_path_layout.addWidget(browse_path_btn)
        output_layout.addLayout(output_path_layout)
        
        # Filename
        filename_layout = QHBoxLayout()
        filename_layout.setSpacing(8)
        filename_layout.addWidget(QLabel("Tên File:"))
        self.recv_filename = QLineEdit("output.txt")
        filename_layout.addWidget(self.recv_filename)
        output_layout.addLayout(filename_layout)
        
        info = QLabel("💡 Ghi chú: Tên file sẽ được ghi đè nếu file đã tồn tại")
        info.setStyleSheet("color: #0078d4; font-size: 9px;")
        output_layout.addWidget(info)
        
        output_group.setLayout(output_layout)
        layout.addWidget(output_group)
        
        # Receive button
        recv_btn = QPushButton("▶️  BẮT ĐẦU LẮNG NGHE")
        recv_btn.setFixedHeight(42)
        recv_btn.setFont(QFont("Arial", 11, QFont.Weight.Bold))
        recv_btn.clicked.connect(self.start_receive)
        layout.addWidget(recv_btn)
        
        layout.addStretch()
        return widget
    
    def apply_styles(self):
        """Apply global styles"""
        self.setStyleSheet("""
            QMainWindow {
                background-color: #f5f5f5;
            }
            QTabWidget::pane {
                border: 1px solid #ddd;
            }
            QTabBar::tab {
                background-color: #e0e0e0;
                padding: 6px 16px;
                margin-right: 2px;
                border: 1px solid #999;
                font-size: 10px;
            }
            QTabBar::tab:selected {
                background-color: #0078d4;
                color: white;
                border: 1px solid #0078d4;
            }
            QGroupBox {
                font-weight: bold;
                font-size: 10px;
                border: 1px solid #ddd;
                border-radius: 4px;
                margin-top: 6px;
                padding-top: 8px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 3px;
            }
            QLineEdit, QTextEdit {
                border: 1px solid #ccc;
                border-radius: 3px;
                padding: 4px;
                font-size: 9px;
            }
            QPushButton {
                background-color: #0078d4;
                color: white;
                border: none;
                border-radius: 3px;
                font-weight: bold;
                font-size: 10px;
                padding: 4px;
            }
            QPushButton:hover {
                background-color: #005a9e;
            }
            QPushButton:pressed {
                background-color: #004578;
            }
        """)
    
    def on_file_selected(self, file_path):
        """Xử lý file được chọn"""
        self.selected_file = file_path
        file_obj = Path(file_path)
        
        # Display file info
        size_mb = file_obj.stat().st_size / (1024 * 1024)
        info_text = f"✅ File: {file_obj.name}\n💾 Kích thước: {size_mb:.2f} MB\n🕐 {datetime.now().strftime('%H:%M:%S')}"
        self.send_file_info.setText(info_text)
        self.log_msg(f"Đã chọn file: {file_obj.name}", "INFO")
    
    def browse_send_file(self):
        """Chọn file để gửi"""
        path, _ = QFileDialog.getOpenFileName(self, "Chọn file để gửi")
        if path:
            self.on_file_selected(path)
    
    def browse_recv_folder(self):
        """Chọn thư mục lưu file"""
        path = QFileDialog.getExistingDirectory(self, "Chọn thư mục lưu file")
        if path:
            self.recv_path.setText(path)
            self.log_msg(f"Thư mục lưu: {path}", "INFO")
    
    def log_msg(self, msg, level="INFO"):
        """Ghi log"""
        icons = {"INFO": "ℹ️", "SUCCESS": "✅", "ERROR": "❌", "WARNING": "⚠️"}
        icon = icons.get(level, "•")
        timestamp = datetime.now().strftime("%H:%M:%S")
        log_line = f"[{timestamp}] [{level}] {icon} {msg}"
        
        cursor = self.log_text.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        cursor.insertText(log_line + "\n")
        self.log_text.setTextCursor(cursor)
    
    def validate_send(self):
        """Validate send inputs"""
        try:
            port = int(self.send_port.text())
            assert 1 <= port <= 65535
        except:
            self.log_msg("Port không hợp lệ (1-65535)", "ERROR")
            return False
        
        if len(self.send_key.text()) < 8:
            self.log_msg("Key phải ≥ 8 ký tự", "ERROR")
            return False
        
        if not self.selected_file or not os.path.exists(self.selected_file):
            self.log_msg("Chọn file trước", "ERROR")
            return False
        
        return True
    
    def validate_receive(self):
        """Validate receive inputs"""
        try:
            port = int(self.recv_port.text())
            assert 1 <= port <= 65535
        except:
            self.log_msg("Port không hợp lệ", "ERROR")
            return False
        
        if len(self.recv_key.text()) < 8:
            self.log_msg("Key phải ≥ 8 ký tự", "ERROR")
            return False
        
        if not os.path.exists(self.recv_path.text()):
            self.log_msg("Thư mục không tồn tại", "ERROR")
            return False
        
        return True
    
    def send_file(self):
        """Gửi file"""
        if not self.validate_send():
            return
        
        self.log_msg("=" * 60, "INFO")
        
        exe_path = self.bin_dir / "client_send.exe"
        if not exe_path.exists():
            self.log_msg(f"Không tìm: {exe_path}", "ERROR")
            return
        
        cmd = [str(exe_path), self.send_ip.text(), self.send_port.text(), 
               self.selected_file, self.send_key.text()]
        
        self.log_msg(f"Gửi: {Path(self.selected_file).name}", "INFO")
        
        self.transfer_thread = TransferThread(cmd, str(self.project_dir))
        self.transfer_thread.log_signal.connect(self.log_msg)
        self.transfer_thread.finished_signal.connect(self.on_transfer_done)
        self.transfer_thread.start()
    
    def start_receive(self):
        """Lắng nghe nhận file"""
        if not self.validate_receive():
            return
        
        self.log_msg("=" * 60, "INFO")
        
        exe_path = self.bin_dir / "server_recv.exe"
        if not exe_path.exists():
            self.log_msg(f"Không tìm: {exe_path}", "ERROR")
            return
        
        output_file = os.path.join(self.recv_path.text(), self.recv_filename.text())
        cmd = [str(exe_path), self.recv_port.text(), output_file, self.recv_key.text()]
        
        self.log_msg(f"Lắng nghe trên port {self.recv_port.text()}...", "INFO")
        
        self.transfer_thread = TransferThread(cmd, str(self.project_dir))
        self.transfer_thread.log_signal.connect(self.log_msg)
        self.transfer_thread.finished_signal.connect(self.on_transfer_done)
        self.transfer_thread.start()
    
    def on_transfer_done(self, success):
        """Transfer xong"""
        if success:
            self.log_msg("Hoàn thành!", "SUCCESS")
        else:
            self.log_msg("Thất bại!", "ERROR")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DESTransferApp()
    window.show()
    sys.exit(app.exec())
