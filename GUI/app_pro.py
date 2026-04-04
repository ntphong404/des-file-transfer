#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DES File Transfer Pro - Multi-File Edition
Gửi/nhận nhiều file với mã hóa DES, hỗ trợ kéo thả nhiều file cùng lúc.
"""

import sys
import os
import subprocess
from pathlib import Path
from datetime import datetime

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QTabWidget, QTextEdit,
    QFileDialog, QGroupBox, QFormLayout, QFrame, QListWidget,
    QListWidgetItem, QAbstractItemView, QSizePolicy, QSpacerItem,
    QScrollArea
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal, QSize, QUrl, QMimeData
from PyQt6.QtGui import (QFont, QColor, QTextCursor, QIcon,
                         QCursor, QDrag, QPixmap)


# ─────────────────────────────────────────────
#  WORKER THREAD
# ─────────────────────────────────────────────
class TransferThread(QThread):
    log_signal      = pyqtSignal(str, str)   # (message, level)
    finished_signal = pyqtSignal(bool)

    def __init__(self, cmd: list, cwd: str):
        super().__init__()
        self.cmd  = cmd
        self.cwd  = cwd
        self.proc: subprocess.Popen | None = None   # reference để kill từ ngoài

    def kill(self):
        """Kill subprocess ngay lập tức (gọi khi đóng app)."""
        if self.proc and self.proc.poll() is None:
            self.proc.kill()

    def run(self):
        try:
            self.proc = subprocess.Popen(
                self.cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding='utf-8',      # C++ stdout là UTF-8, không dùng system encoding
                errors='replace',      # ký tự lỗi → '?' thay vì crash
                cwd=self.cwd
            )
            for line in self.proc.stdout:
                line = line.rstrip()
                if line:
                    level = "ERROR" if any(k in line.lower() for k in ("error", "fail", "invalid")) else "INFO"
                    if "ok" in line.lower() or "success" in line.lower() or "[OK]" in line:
                        level = "SUCCESS"
                    if "warning" in line.lower():
                        level = "WARNING"
                    self.log_signal.emit(line, level)

            self.proc.wait()
            if self.proc.returncode == 0:
                self.log_signal.emit("✅ Hoàn thành!", "SUCCESS")
                self.finished_signal.emit(True)
            else:
                self.log_signal.emit(f"❌ Lỗi! Exit code: {self.proc.returncode}", "ERROR")
                self.finished_signal.emit(False)

        except Exception as e:
            self.log_signal.emit(f"❌ Exception: {e}", "ERROR")
            self.finished_signal.emit(False)


# ─────────────────────────────────────────────
#  MULTI-FILE DROP ZONE
# ─────────────────────────────────────────────
class MultiFileDropZone(QFrame):
    """
    Khu vực kéo thả nhiều file cùng lúc.
    Emit: files_added(list[str])
    """
    files_added = pyqtSignal(list)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAcceptDrops(True)
        self.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self._is_dragging = False
        self._build_ui()
        self._update_style(False)

    def _build_ui(self):
        layout = QVBoxLayout(self)
        layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.setContentsMargins(20, 20, 20, 20)
        layout.setSpacing(6)

        icon_label = QLabel("📂")
        icon_label.setFont(QFont("Segoe UI Emoji", 28))
        icon_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        title = QLabel("Kéo & thả file vào đây")
        title.setFont(QFont("Segoe UI", 11, QFont.Weight.Bold))
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        title.setStyleSheet("color: #334155;")

        sub = QLabel("Hỗ trợ nhiều file cùng lúc")
        sub.setFont(QFont("Segoe UI", 8))
        sub.setAlignment(Qt.AlignmentFlag.AlignCenter)
        sub.setStyleSheet("color: #94a3b8;")

        sep = QLabel("— hoặc —")
        sep.setFont(QFont("Segoe UI", 8))
        sep.setAlignment(Qt.AlignmentFlag.AlignCenter)
        sep.setStyleSheet("color: #94a3b8;")

        btn = QPushButton("  Chọn File...")
        btn.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        btn.setFixedWidth(130)
        btn.setFixedHeight(32)
        btn.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        btn.setStyleSheet("""
            QPushButton {
                background-color: #3b82f6;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 4px 12px;
            }
            QPushButton:hover { background-color: #2563eb; }
            QPushButton:pressed { background-color: #1d4ed8; }
        """)
        btn.clicked.connect(self._browse_files)

        layout.addWidget(icon_label)
        layout.addWidget(title)
        layout.addWidget(sub)
        layout.addWidget(sep)
        layout.addWidget(btn, alignment=Qt.AlignmentFlag.AlignCenter)

    def _browse_files(self):
        paths, _ = QFileDialog.getOpenFileNames(
            self, "Chọn file để gửi", "", "Tất cả file (*.*)"
        )
        if paths:
            self.files_added.emit(paths)

    def _update_style(self, hovering: bool):
        if hovering:
            bg = "#eff6ff"
            border = "2px dashed #3b82f6"
        else:
            bg = "#f8fafc"
            border = "2px dashed #cbd5e1"
        self.setStyleSheet(f"""
            MultiFileDropZone {{
                background-color: {bg};
                border: {border};
                border-radius: 10px;
            }}
        """)

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
            self._update_style(True)

    def dragLeaveEvent(self, event):
        self._update_style(False)

    def dropEvent(self, event):
        self._update_style(False)
        paths = [url.toLocalFile() for url in event.mimeData().urls()
                 if url.isLocalFile()]
        if paths:
            self.files_added.emit(paths)


# ─────────────────────────────────────────────
#  FILE LIST WIDGET
# ─────────────────────────────────────────────
class FileListWidget(QWidget):
    """Danh sách file đã chọn, có thể xoá từng file."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._files: list[str] = []   # full paths
        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(6)

        # Header bar
        header = QHBoxLayout()
        self.count_label = QLabel("0 file đã chọn")
        self.count_label.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        self.count_label.setStyleSheet("color: #475569;")

        self.clear_btn = QPushButton("🗑 Xoá tất cả")
        self.clear_btn.setFixedHeight(26)
        self.clear_btn.setFont(QFont("Segoe UI", 8))
        self.clear_btn.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.clear_btn.setStyleSheet("""
            QPushButton {
                background-color: #fee2e2;
                color: #dc2626;
                border: 1px solid #fca5a5;
                border-radius: 5px;
                padding: 2px 8px;
            }
            QPushButton:hover { background-color: #fecaca; }
        """)
        self.clear_btn.clicked.connect(self.clear_all)

        header.addWidget(self.count_label)
        header.addStretch()
        header.addWidget(self.clear_btn)

        # List widget
        self.list_widget = QListWidget()
        self.list_widget.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.list_widget.setAlternatingRowColors(True)
        self.list_widget.setFont(QFont("Segoe UI", 9))
        self.list_widget.setStyleSheet("""
            QListWidget {
                border: 1px solid #e2e8f0;
                border-radius: 6px;
                background-color: white;
                outline: none;
            }
            QListWidget::item {
                padding: 5px 8px;
                border-bottom: 1px solid #f1f5f9;
                color: #1e293b;
            }
            QListWidget::item:selected {
                background-color: #dbeafe;
                color: #1e40af;
            }
            QListWidget::item:alternate {
                background-color: #f8fafc;
            }
        """)

        # Remove selected button
        self.remove_btn = QPushButton("✕ Xoá file đã chọn")
        self.remove_btn.setFixedHeight(28)
        self.remove_btn.setFont(QFont("Segoe UI", 8))
        self.remove_btn.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.remove_btn.setStyleSheet("""
            QPushButton {
                background-color: #fff7ed;
                color: #ea580c;
                border: 1px solid #fed7aa;
                border-radius: 5px;
                padding: 2px 10px;
            }
            QPushButton:hover { background-color: #ffedd5; }
        """)
        self.remove_btn.clicked.connect(self._remove_selected)

        root.addLayout(header)
        root.addWidget(self.list_widget)
        root.addWidget(self.remove_btn, alignment=Qt.AlignmentFlag.AlignRight)

        self._refresh_ui()

    def add_files(self, paths: list[str]):
        """Thêm các file vào danh sách (bỏ trùng)."""
        added = 0
        for p in paths:
            p = os.path.normpath(p)
            if os.path.isfile(p) and p not in self._files:
                self._files.append(p)
                added += 1
        if added:
            self._refresh_ui()

    def clear_all(self):
        self._files.clear()
        self._refresh_ui()

    def _remove_selected(self):
        selected_rows = sorted(
            [self.list_widget.row(item) for item in self.list_widget.selectedItems()],
            reverse=True
        )
        for row in selected_rows:
            del self._files[row]
        self._refresh_ui()

    def _refresh_ui(self):
        self.list_widget.clear()
        for path in self._files:
            name = os.path.basename(path)
            size = os.path.getsize(path) if os.path.exists(path) else 0
            size_str = self._fmt_size(size)
            item = QListWidgetItem(f"  📄  {name}   ({size_str})")
            item.setToolTip(path)
            self.list_widget.addItem(item)

        count = len(self._files)
        self.count_label.setText(
            f"{count} file đã chọn" if count else "Chưa chọn file nào"
        )
        self.count_label.setStyleSheet(
            f"color: {'#16a34a' if count else '#475569'}; font-weight: bold;"
        )
        self.clear_btn.setVisible(count > 0)
        self.remove_btn.setVisible(count > 0)

    @staticmethod
    def _fmt_size(size: int) -> str:
        if size < 1024:
            return f"{size} B"
        elif size < 1024 * 1024:
            return f"{size / 1024:.1f} KB"
        elif size < 1024 * 1024 * 1024:
            return f"{size / 1024 / 1024:.2f} MB"
        return f"{size / 1024 / 1024 / 1024:.2f} GB"

    @property
    def files(self) -> list[str]:
        return list(self._files)

    @property
    def count(self) -> int:
        return len(self._files)


# ─────────────────────────────────────────────
#  MAIN APPLICATION WINDOW
# ─────────────────────────────────────────────
class DESTransferApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("🔐 DES File Transfer Pro")

        # Tự động điều chỉnh theo màn hình
        screen = QApplication.primaryScreen().availableGeometry()
        win_w = min(820, screen.width() - 40)
        win_h = int(screen.height() * 0.85)
        x = (screen.width()  - win_w) // 2
        y = (screen.height() - win_h) // 2
        self.setGeometry(x, y, win_w, win_h)
        self.setMinimumSize(700, 500)

        self.app_dir     = Path(__file__).parent
        self.project_dir = self.app_dir.parent / "SecureFileTransfer"
        self.bin_dir     = self.project_dir / "bin"

        self.transfer_thread: TransferThread | None = None

        self._build_ui()
        self._apply_global_styles()

    def closeEvent(self, event):
        """Đóng app → kill subprocess + dừng QThread sạch sẽ."""
        if self.transfer_thread is not None and self.transfer_thread.isRunning():
            self.transfer_thread.kill()          # kill process con (exe C++)
            self.transfer_thread.quit()           # báo QThread thoát
            self.transfer_thread.wait(3000)       # chờ tối đa 3 giây
            if self.transfer_thread.isRunning():
                self.transfer_thread.terminate()  # force terminate nếu vẫn còn
        event.accept()

    # ── UI BUILD ─────────────────────────────
    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        root = QVBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        # Header (fixed, không cuộn)
        root.addWidget(self._make_header())

        # ── Một QScrollArea bọc toàn bộ body (tabs + log) ──
        main_scroll = QScrollArea()
        main_scroll.setWidgetResizable(True)
        main_scroll.setFrameShape(QFrame.Shape.NoFrame)
        main_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        main_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        main_scroll.setStyleSheet(
            "QScrollArea { background-color: #f1f5f9; border: none; }"
        )

        body = QWidget()
        body.setStyleSheet("background-color: #f1f5f9;")
        body_layout = QVBoxLayout(body)
        body_layout.setContentsMargins(12, 12, 12, 12)
        body_layout.setSpacing(10)

        self.tabs = QTabWidget()
        self.tabs.setFont(QFont("Segoe UI", 10))
        self.tabs.addTab(self._make_send_tab(),    "📤  Gửi File")
        self.tabs.addTab(self._make_receive_tab(), "📥  Nhận File")
        body_layout.addWidget(self.tabs)

        # Log panel nằm trong scroll area
        body_layout.addWidget(self._make_log_panel())
        body_layout.addStretch(1)

        main_scroll.setWidget(body)
        root.addWidget(main_scroll)

    def _make_header(self) -> QWidget:
        header = QWidget()
        header.setFixedHeight(72)
        header.setStyleSheet(
            "background: qlineargradient("
            "x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #1e40af, stop:1 #3b82f6);"
        )
        layout = QVBoxLayout(header)
        layout.setContentsMargins(20, 8, 20, 8)
        layout.setSpacing(2)

        t = QLabel("🔐  DES FILE TRANSFER PRO")
        t.setFont(QFont("Segoe UI", 16, QFont.Weight.Bold))
        t.setStyleSheet("color: white; background: transparent;")

        s = QLabel("Mã hóa DES · Truyền nhiều file qua TCP/IP · Tự động lưu tên file")
        s.setFont(QFont("Segoe UI", 8))
        s.setStyleSheet("color: #bfdbfe; background: transparent;")

        layout.addWidget(t)
        layout.addWidget(s)
        return header

    # ── SEND TAB ─────────────────────────────
    def _make_send_tab(self) -> QWidget:
        w = QWidget()
        w.setStyleSheet("background: transparent;")
        layout = QVBoxLayout(w)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)

        # Config group
        cfg = QGroupBox("⚙️   Cấu Hình Kết Nối")
        cfg_layout = QFormLayout()
        cfg_layout.setVerticalSpacing(8)
        cfg_layout.setHorizontalSpacing(14)

        self.send_ip   = QLineEdit("127.0.0.1")
        self.send_port = QLineEdit("5000")
        self.send_key  = QLineEdit("12345678")
        self.send_key.setEchoMode(QLineEdit.EchoMode.Password)

        for w_ in (self.send_ip, self.send_port, self.send_key):
            w_.setFixedHeight(32)

        cfg_layout.addRow("IP Server:", self.send_ip)
        cfg_layout.addRow("Port:", self.send_port)
        cfg_layout.addRow("Key (mật khẩu):", self.send_key)
        cfg.setLayout(cfg_layout)
        layout.addWidget(cfg)

        # Drop zone
        drop_group = QGroupBox("📁  Chọn File")
        drop_layout = QVBoxLayout()
        drop_layout.setContentsMargins(10, 10, 10, 10)
        drop_layout.setSpacing(8)

        self.drop_zone = MultiFileDropZone()
        self.drop_zone.setMinimumHeight(130)
        self.drop_zone.files_added.connect(self._on_files_added)

        self.file_list = FileListWidget()
        self.file_list.setMinimumHeight(150)

        drop_layout.addWidget(self.drop_zone)
        drop_layout.addWidget(self.file_list)
        drop_group.setLayout(drop_layout)
        layout.addWidget(drop_group)

        # Send button
        self.send_btn = QPushButton("▶   GỬI FILE")
        self.send_btn.setFixedHeight(46)
        self.send_btn.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        self.send_btn.clicked.connect(self._send_files)
        layout.addWidget(self.send_btn)

        return w

    # ── RECEIVE TAB ──────────────────────────
    def _make_receive_tab(self) -> QWidget:
        w = QWidget()
        w.setStyleSheet("background: transparent;")
        layout = QVBoxLayout(w)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)

        # Config group
        cfg = QGroupBox("⚙️   Cấu Hình Server")
        cfg_layout = QFormLayout()
        cfg_layout.setVerticalSpacing(8)
        cfg_layout.setHorizontalSpacing(14)

        self.recv_port = QLineEdit("5000")
        self.recv_key  = QLineEdit("12345678")
        self.recv_key.setEchoMode(QLineEdit.EchoMode.Password)

        for w_ in (self.recv_port, self.recv_key):
            w_.setFixedHeight(32)

        cfg_layout.addRow("Port lắng nghe:", self.recv_port)
        cfg_layout.addRow("Key (mật khẩu):", self.recv_key)
        cfg.setLayout(cfg_layout)
        layout.addWidget(cfg)

        # Output directory group
        out_group = QGroupBox("💾  Thư Mục Lưu File")
        out_layout = QVBoxLayout()
        out_layout.setContentsMargins(10, 12, 10, 10)
        out_layout.setSpacing(8)

        dir_row = QHBoxLayout()
        dir_row.setSpacing(8)
        self.recv_dir = QLineEdit(str(self.project_dir / "received"))
        self.recv_dir.setFixedHeight(32)
        browse_btn = QPushButton("📂 Chọn thư mục")
        browse_btn.setFixedHeight(32)
        browse_btn.setFixedWidth(130)
        browse_btn.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        browse_btn.clicked.connect(self._browse_recv_dir)
        dir_row.addWidget(self.recv_dir)
        dir_row.addWidget(browse_btn)
        out_layout.addLayout(dir_row)

        note = QLabel(
            "💡 File nhận sẽ được lưu tự động với tên gốc từ bên gửi. "
            "Thư mục sẽ được tạo nếu chưa tồn tại."
        )
        note.setWordWrap(True)
        note.setStyleSheet("color: #3b82f6; font-size: 9px; padding: 4px 0;")
        out_layout.addWidget(note)

        out_group.setLayout(out_layout)
        layout.addWidget(out_group)

        # ── Panel file đã nhận ──
        status_box = QGroupBox("📊  Kết Quả Nhận")
        sb_layout = QVBoxLayout()
        sb_layout.setContentsMargins(10, 10, 10, 10)
        sb_layout.setSpacing(6)

        # Status badge (1 dòng)
        self.recv_status = QLabel("🔵 Chưa bắt đầu")
        self.recv_status.setFont(QFont("Segoe UI", 9, QFont.Weight.Bold))
        self.recv_status.setStyleSheet("color: #64748b;")
        self.recv_status.setWordWrap(True)
        sb_layout.addWidget(self.recv_status)

        # Header hàng file
        file_header = QHBoxLayout()
        self.recv_count_label = QLabel("Danh sách file đã nhận:")
        self.recv_count_label.setFont(QFont("Segoe UI", 8))
        self.recv_count_label.setStyleSheet("color: #94a3b8;")
        self.recv_clear_btn = QPushButton("🗑 Xóa")
        self.recv_clear_btn.setFixedHeight(22)
        self.recv_clear_btn.setFixedWidth(55)
        self.recv_clear_btn.setFont(QFont("Segoe UI", 7))
        self.recv_clear_btn.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.recv_clear_btn.setStyleSheet("""
            QPushButton {
                background-color: #fee2e2; color: #dc2626;
                border: 1px solid #fca5a5; border-radius: 4px;
            }
            QPushButton:hover { background-color: #fecaca; }
        """)
        self.recv_clear_btn.clicked.connect(self._clear_recv_list)
        file_header.addWidget(self.recv_count_label)
        file_header.addStretch()
        file_header.addWidget(self.recv_clear_btn)
        sb_layout.addLayout(file_header)

        # Danh sách file nhận được
        self.recv_file_list = QListWidget()
        self.recv_file_list.setFixedHeight(150)
        self.recv_file_list.setFont(QFont("Segoe UI", 9))
        self.recv_file_list.setStyleSheet("""
            QListWidget {
                border: 1px solid #e2e8f0;
                border-radius: 6px;
                background-color: #f8fafc;
                outline: none;
            }
            QListWidget::item {
                padding: 5px 8px;
                border-bottom: 1px solid #f1f5f9;
            }
            QListWidget::item:selected {
                background-color: #dcfce7;
                color: #15803d;
            }
        """)
        sb_layout.addWidget(self.recv_file_list)

        status_box.setLayout(sb_layout)
        layout.addWidget(status_box)

        layout.addStretch()

        # Listen button
        self.recv_btn = QPushButton("▶   BẮT ĐẦU LẮNG NGHE")
        self.recv_btn.setFixedHeight(46)
        self.recv_btn.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        self.recv_btn.clicked.connect(self._start_receive)
        layout.addWidget(self.recv_btn)

        return w

    # ── LOG PANEL ────────────────────────────
    def _make_log_panel(self) -> QGroupBox:
        box = QGroupBox("📋  Nhật Ký Hoạt Động")
        layout = QVBoxLayout()
        layout.setContentsMargins(8, 8, 8, 8)
        layout.setSpacing(4)

        self.log_box = QTextEdit()
        self.log_box.setReadOnly(True)
        self.log_box.setFont(QFont("Consolas", 8))
        self.log_box.setFixedHeight(130)
        self.log_box.setStyleSheet("""
            QTextEdit {
                background-color: #0f172a;
                color: #e2e8f0;
                border: none;
                border-radius: 6px;
                padding: 6px;
            }
        """)

        clear_log = QPushButton("Xoá log")
        clear_log.setFixedHeight(22)
        clear_log.setFixedWidth(68)
        clear_log.setFont(QFont("Segoe UI", 7))
        clear_log.setStyleSheet("""
            QPushButton {
                background-color: #334155;
                color: #94a3b8;
                border: none;
                border-radius: 4px;
            }
            QPushButton:hover { background-color: #475569; }
        """)
        clear_log.clicked.connect(self.log_box.clear)

        layout.addWidget(self.log_box)
        layout.addWidget(clear_log, alignment=Qt.AlignmentFlag.AlignRight)
        box.setLayout(layout)
        return box

    # ── GLOBAL STYLES ────────────────────────
    def _apply_global_styles(self):
        self.setStyleSheet("""
            QMainWindow { background-color: #f1f5f9; }

            QTabWidget::pane {
                border: 1px solid #e2e8f0;
                border-radius: 8px;
                background-color: white;
            }
            QTabBar::tab {
                background-color: #e2e8f0;
                color: #475569;
                padding: 8px 20px;
                margin-right: 3px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                font-family: 'Segoe UI';
                font-size: 10px;
                font-weight: bold;
            }
            QTabBar::tab:selected {
                background-color: white;
                color: #1e40af;
                border-bottom: 2px solid #3b82f6;
            }
            QTabBar::tab:hover:!selected { background-color: #dbeafe; }

            QGroupBox {
                font-family: 'Segoe UI';
                font-weight: bold;
                font-size: 10px;
                color: #334155;
                border: 1px solid #e2e8f0;
                border-radius: 8px;
                margin-top: 8px;
                padding-top: 10px;
                background-color: white;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 4px;
                background-color: white;
            }

            QLineEdit {
                border: 1.5px solid #cbd5e1;
                border-radius: 6px;
                padding: 4px 10px;
                font-family: 'Segoe UI';
                font-size: 9px;
                color: #1e293b;
                background-color: white;
            }
            QLineEdit:focus { border-color: #3b82f6; }

            QPushButton#send_btn, QPushButton#recv_btn {
                background-color: #2563eb;
                color: white;
                border: none;
                border-radius: 8px;
                font-weight: bold;
            }
            QPushButton#send_btn:hover, QPushButton#recv_btn:hover {
                background-color: #1d4ed8;
            }
            QPushButton#send_btn:pressed, QPushButton#recv_btn:pressed {
                background-color: #1e40af;
            }
            QPushButton#send_btn:disabled, QPushButton#recv_btn:disabled {
                background-color: #94a3b8;
            }
        """)

        # Name the main action buttons for targeted CSS
        self.send_btn.setObjectName("send_btn")
        self.recv_btn.setObjectName("recv_btn")
        self.send_btn.setStyleSheet("""
            QPushButton#send_btn {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 #2563eb, stop:1 #3b82f6);
                color: white;
                border: none;
                border-radius: 8px;
            }
            QPushButton#send_btn:hover {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 #1d4ed8, stop:1 #2563eb);
            }
            QPushButton#send_btn:disabled { background: #94a3b8; }
        """)
        self.recv_btn.setStyleSheet("""
            QPushButton#recv_btn {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 #059669, stop:1 #10b981);
                color: white;
                border: none;
                border-radius: 8px;
            }
            QPushButton#recv_btn:hover {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                    stop:0 #047857, stop:1 #059669);
            }
            QPushButton#recv_btn:disabled { background: #94a3b8; }
        """)

    # ── SLOTS ────────────────────────────────
    def _on_files_added(self, paths: list):
        self.file_list.add_files(paths)
        self._log(f"Thêm {len(paths)} file vào danh sách.", "INFO")

    def _browse_recv_dir(self):
        path = QFileDialog.getExistingDirectory(self, "Chọn thư mục lưu file")
        if path:
            self.recv_dir.setText(path)
            self._log(f"Thư mục nhận: {path}", "INFO")

    # ── SEND ─────────────────────────────────
    def _send_files(self):
        if self.transfer_thread and self.transfer_thread.isRunning():
            self._log("Đang có tiến trình chạy, vui lòng đợi!", "WARNING")
            return

        if not self._validate_send():
            return

        exe = self.bin_dir / "client_send.exe"
        if not exe.exists():
            self._log(f"Không tìm thấy: {exe}", "ERROR")
            return

        files = self.file_list.files
        key8  = self.send_key.text()[:8]   # only first 8 chars

        cmd = [
            str(exe),
            self.send_ip.text().strip(),
            self.send_port.text().strip(),
            key8,
            *files
        ]

        self._log("=" * 55, "INFO")
        self._log(f"Gửi {len(files)} file đến {self.send_ip.text()}:{self.send_port.text()}", "INFO")
        for f in files:
            self._log(f"  • {os.path.basename(f)}", "INFO")

        self.send_btn.setEnabled(False)
        self.send_btn.setText("⏳  Đang gửi...")

        self.transfer_thread = TransferThread(cmd, str(self.project_dir))
        self.transfer_thread.log_signal.connect(self._log)
        self.transfer_thread.finished_signal.connect(self._on_send_done)
        self.transfer_thread.start()

    def _on_send_done(self, success: bool):
        self.send_btn.setEnabled(True)
        self.send_btn.setText("▶   GỬI FILE")
        if success:
            self._log("📦 Tất cả file đã gửi thành công!", "SUCCESS")
        else:
            self._log("Gửi file thất bại. Kiểm tra kết nối và key.", "ERROR")

    # ── RECEIVE ──────────────────────────────
    def _start_receive(self):
        if self.transfer_thread and self.transfer_thread.isRunning():
            self._log("Đang có tiến trình chạy, vui lòng đợi!", "WARNING")
            return

        if not self._validate_receive():
            return

        exe = self.bin_dir / "server_recv.exe"
        if not exe.exists():
            self._log(f"Không tìm thấy: {exe}", "ERROR")
            return

        key8    = self.recv_key.text()[:8]
        out_dir = self.recv_dir.text().strip()

        cmd = [str(exe), self.recv_port.text().strip(), out_dir, key8]

        self._log("=" * 55, "INFO")
        self._log(f"Lắng nghe trên port {self.recv_port.text()}...", "INFO")
        self._log(f"File sẽ lưu vào: {out_dir}", "INFO")

        self.recv_btn.setEnabled(False)
        self.recv_btn.setText("⏳  Đang lắng nghe...")

        # Reset panel file nhận
        self._clear_recv_list()
        self.recv_status.setText(
            f"🔵 Đang lắng nghe trên port {self.recv_port.text()}...\n"
            "Chờ client kết nối..."
        )
        self.recv_status.setStyleSheet("color: #2563eb; font-weight: bold;")

        self.transfer_thread = TransferThread(cmd, str(self.project_dir))
        self.transfer_thread.log_signal.connect(self._log)
        self.transfer_thread.log_signal.connect(self._on_recv_log)   # parse file list
        self.transfer_thread.finished_signal.connect(self._on_recv_done)
        self.transfer_thread.start()

    def _on_recv_log(self, msg: str, level: str):
        """Parse log lines từ server để hiển thị danh sách file đã nhận."""
        # Server in: "  Saved to  : /path/to/file.txt  [OK]"
        low = msg.strip().lower()
        if "saved to" in low and "[ok]" in low:
            # Tách phần path giữa ":" và "[OK]"
            try:
                part = msg.split(":", 1)[1]          # bỏ phần "Saved to"
                path = part.replace("[OK]", "").strip()
                name = os.path.basename(path)
                size_str = ""
                if os.path.exists(path):
                    sz = os.path.getsize(path)
                    size_str = FileListWidget._fmt_size(sz)
                item = QListWidgetItem(f"  ✅  {name}   {size_str}")
                item.setToolTip(path)
                item.setForeground(QColor("#15803d"))
                self.recv_file_list.addItem(item)
                self.recv_file_list.scrollToBottom()
                n = self.recv_file_list.count()
                self.recv_count_label.setText(
                    f"Danh sách file đã nhận: ✔ {n} file"
                )
                self.recv_count_label.setStyleSheet("color: #16a34a; font-size: 8px; font-weight: bold;")
            except Exception:
                pass

    def _clear_recv_list(self):
        self.recv_file_list.clear()
        self.recv_count_label.setText("Danh sách file đã nhận:")
        self.recv_count_label.setStyleSheet("color: #94a3b8; font-size: 8px;")
        self.recv_status.setText("🔵 Chưa bắt đầu")
        self.recv_status.setStyleSheet("color: #64748b; font-weight: bold;")

    def _on_recv_done(self, success: bool):
        self.recv_btn.setEnabled(True)
        self.recv_btn.setText("▶   BẮT ĐẦU LẮNG NGHE")
        if success:
            n = self.recv_file_list.count()
            self.recv_status.setText(
                f"✅ Nhận thành công {n} file!"
            )
            self.recv_status.setStyleSheet("color: #16a34a; font-weight: bold;")
            self._log(f"✅ Nhận xong {n} file!", "SUCCESS")
        else:
            self.recv_status.setText("❌ Nhận file thất bại. Kiểm tra key và kết nối.")
            self.recv_status.setStyleSheet("color: #dc2626; font-weight: bold;")
            self._log("Nhận file thất bại.", "ERROR")

    # ── VALIDATION ────────────────────────────
    def _validate_send(self) -> bool:
        try:
            p = int(self.send_port.text())
            assert 1 <= p <= 65535
        except Exception:
            self._log("Port không hợp lệ (1–65535)", "ERROR")
            return False

        if not self.send_ip.text().strip():
            self._log("IP Server không được để trống", "ERROR")
            return False

        if len(self.send_key.text()) < 8:
            self._log("Key phải có ít nhất 8 ký tự", "ERROR")
            return False

        if self.file_list.count == 0:
            self._log("Chưa chọn file nào để gửi!", "ERROR")
            return False

        # Check all files still exist
        bad = [f for f in self.file_list.files if not os.path.exists(f)]
        if bad:
            self._log(f"Không tìm thấy {len(bad)} file: {', '.join(os.path.basename(b) for b in bad)}", "ERROR")
            return False

        return True

    def _validate_receive(self) -> bool:
        try:
            p = int(self.recv_port.text())
            assert 1 <= p <= 65535
        except Exception:
            self._log("Port không hợp lệ (1–65535)", "ERROR")
            return False

        if len(self.recv_key.text()) < 8:
            self._log("Key phải có ít nhất 8 ký tự", "ERROR")
            return False

        d = self.recv_dir.text().strip()
        if not d:
            self._log("Cần nhập thư mục lưu file", "ERROR")
            return False

        return True

    # ── LOGGING ──────────────────────────────
    def _log(self, msg: str, level: str = "INFO"):
        colors = {
            "INFO":    "#94a3b8",
            "SUCCESS": "#4ade80",
            "ERROR":   "#f87171",
            "WARNING": "#fbbf24",
        }
        prefix = {
            "INFO":    "·",
            "SUCCESS": "✓",
            "ERROR":   "✗",
            "WARNING": "⚠",
        }
        color = colors.get(level, "#94a3b8")
        icon  = prefix.get(level, "·")
        ts    = datetime.now().strftime("%H:%M:%S")

        cursor = self.log_box.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.End)
        # Use HTML for color
        html = (
            f'<span style="color:#475569;">[{ts}]</span> '
            f'<span style="color:{color};">{icon} {msg}</span><br>'
        )
        cursor.insertHtml(html)
        self.log_box.setTextCursor(cursor)
        self.log_box.ensureCursorVisible()


# ─────────────────────────────────────────────
if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    window = DESTransferApp()
    window.show()
    sys.exit(app.exec())
