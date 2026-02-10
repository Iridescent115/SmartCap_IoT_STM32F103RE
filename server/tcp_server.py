"""
TCP/IPv6 服务器 - 用于与RG200U 4G模块通信
支持IPv6公网地址，TCP长连接透传
"""

import sys
import socket
import threading
from datetime import datetime
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QTextEdit, QLineEdit, QPushButton, 
                             QLabel, QGroupBox)
from PyQt5.QtCore import pyqtSignal, QObject
from PyQt5.QtGui import QFont, QTextCursor


class ServerSignals(QObject):
    """信号类，用于线程间通信"""
    log_message = pyqtSignal(str)
    client_connected = pyqtSignal(str)
    client_disconnected = pyqtSignal(str)
    data_received = pyqtSignal(str)


class TCPServer(QMainWindow):
    def __init__(self):
        super().__init__()
        self.signals = ServerSignals()
        self.server_socket = None
        self.client_socket = None
        self.client_address = None
        self.server_thread = None
        self.running = False
        
        self.init_ui()
        self.connect_signals()
        
    def init_ui(self):
        """初始化UI"""
        self.setWindowTitle('RG200U TCP/IPv6 服务器')
        self.setGeometry(100, 100, 800, 600)
        
        # 主窗口部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # 服务器配置区域
        config_group = QGroupBox("服务器配置")
        config_layout = QHBoxLayout()
        
        config_layout.addWidget(QLabel("监听地址:"))
        self.addr_input = QLineEdit("::")  # IPv6任意地址
        self.addr_input.setFixedWidth(300)
        config_layout.addWidget(self.addr_input)
        
        config_layout.addWidget(QLabel("端口:"))
        self.port_input = QLineEdit("18655")
        self.port_input.setFixedWidth(80)
        config_layout.addWidget(self.port_input)
        
        self.start_btn = QPushButton("启动服务器")
        self.start_btn.clicked.connect(self.toggle_server)
        config_layout.addWidget(self.start_btn)
        
        config_layout.addStretch()
        config_group.setLayout(config_layout)
        main_layout.addWidget(config_group)
        
        # 日志显示区域
        log_group = QGroupBox("服务器日志")
        log_layout = QVBoxLayout()
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        self.log_text.setFont(QFont("Consolas", 9))
        log_layout.addWidget(self.log_text)
        
        log_group.setLayout(log_layout)
        main_layout.addWidget(log_group)
        
        # 接收数据显示区域
        recv_group = QGroupBox("接收的数据")
        recv_layout = QVBoxLayout()
        
        self.recv_text = QTextEdit()
        self.recv_text.setReadOnly(True)
        self.recv_text.setFont(QFont("Consolas", 10))
        recv_layout.addWidget(self.recv_text)
        
        recv_group.setLayout(recv_layout)
        main_layout.addWidget(recv_group)
        
        # 发送数据区域
        send_group = QGroupBox("发送数据")
        send_layout = QVBoxLayout()
        
        send_input_layout = QHBoxLayout()
        self.send_input = QLineEdit()
        self.send_input.setPlaceholderText("输入要发送的数据...")
        self.send_input.returnPressed.connect(self.send_data)
        send_input_layout.addWidget(self.send_input)
        
        self.send_btn = QPushButton("发送")
        self.send_btn.clicked.connect(self.send_data)
        self.send_btn.setEnabled(False)
        send_input_layout.addWidget(self.send_btn)
        
        send_layout.addLayout(send_input_layout)
        
        # 快捷发送按钮
        quick_layout = QHBoxLayout()
        quick_layout.addWidget(QLabel("快捷发送:"))
        
        btn_test = QPushButton("测试消息")
        btn_test.clicked.connect(lambda: self.quick_send("Hello from Server!"))
        quick_layout.addWidget(btn_test)
        
        btn_relay_on = QPushButton("继电器1开")
        btn_relay_on.clicked.connect(lambda: self.quick_send("RELAY1_ON"))
        quick_layout.addWidget(btn_relay_on)
        
        btn_relay_off = QPushButton("继电器1关")
        btn_relay_off.clicked.connect(lambda: self.quick_send("RELAY1_OFF"))
        quick_layout.addWidget(btn_relay_off)
        
        quick_layout.addStretch()
        send_layout.addLayout(quick_layout)
        
        send_group.setLayout(send_layout)
        main_layout.addWidget(send_group)
        
        # 设置布局比例
        main_layout.setStretch(1, 2)  # 日志区域
        main_layout.setStretch(2, 3)  # 接收区域
        main_layout.setStretch(3, 2)  # 发送区域
        
    def connect_signals(self):
        """连接信号到槽函数"""
        self.signals.log_message.connect(self.append_log)
        self.signals.client_connected.connect(self.on_client_connected)
        self.signals.client_disconnected.connect(self.on_client_disconnected)
        self.signals.data_received.connect(self.append_received_data)
        
    def toggle_server(self):
        """启动/停止服务器"""
        if not self.running:
            self.start_server()
        else:
            self.stop_server()
            
    def start_server(self):
        """启动TCP服务器"""
        try:
            addr = self.addr_input.text()
            port = int(self.port_input.text())
            
            # 创建IPv6 TCP socket
            self.server_socket = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            # 绑定地址和端口
            self.server_socket.bind((addr, port))
            self.server_socket.listen(1)
            
            self.running = True
            self.start_btn.setText("停止服务器")
            self.start_btn.setStyleSheet("background-color: #ff6b6b")
            self.addr_input.setEnabled(False)
            self.port_input.setEnabled(False)
            
            self.signals.log_message.emit(f"[启动] 服务器已启动，监听 [{addr}]:{port}")
            
            # 启动服务器线程
            self.server_thread = threading.Thread(target=self.server_loop, daemon=True)
            self.server_thread.start()
            
        except Exception as e:
            self.signals.log_message.emit(f"[错误] 启动失败: {str(e)}")
            self.running = False
            
    def stop_server(self):
        """停止TCP服务器"""
        self.running = False
        
        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass
            self.client_socket = None
            
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
            self.server_socket = None
            
        self.start_btn.setText("启动服务器")
        self.start_btn.setStyleSheet("")
        self.addr_input.setEnabled(True)
        self.port_input.setEnabled(True)
        self.send_btn.setEnabled(False)
        
        self.signals.log_message.emit("[停止] 服务器已停止")
        
    def server_loop(self):
        """服务器主循环"""
        while self.running:
            try:
                self.signals.log_message.emit("[等待] 等待客户端连接...")
                
                # 接受客户端连接
                self.client_socket, self.client_address = self.server_socket.accept()
                
                # 提取IPv6地址
                client_ip = self.client_address[0]
                client_port = self.client_address[1]
                
                self.signals.client_connected.emit(f"{client_ip}:{client_port}")
                
                # 接收数据循环
                while self.running:
                    try:
                        data = self.client_socket.recv(4096)
                        if not data:
                            break
                            
                        # 解码数据（尝试UTF-8，失败则显示十六进制）
                        try:
                            text = data.decode('utf-8')
                            self.signals.data_received.emit(f"[文本] {text}")
                        except:
                            hex_str = ' '.join([f'{b:02X}' for b in data])
                            self.signals.data_received.emit(f"[HEX] {hex_str}")
                            
                    except socket.timeout:
                        continue
                    except Exception as e:
                        self.signals.log_message.emit(f"[错误] 接收数据失败: {str(e)}")
                        break
                        
            except Exception as e:
                if self.running:
                    self.signals.log_message.emit(f"[错误] 连接异常: {str(e)}")
                    
            finally:
                if self.client_socket:
                    self.client_socket.close()
                    self.client_socket = None
                    self.signals.client_disconnected.emit("")
                    
    def send_data(self):
        """发送数据到客户端"""
        if not self.client_socket:
            self.signals.log_message.emit("[错误] 没有连接的客户端")
            return
            
        text = self.send_input.text()
        if not text:
            return
            
        try:
            # 发送数据（UTF-8编码）
            data = text.encode('utf-8')
            self.client_socket.sendall(data)
            
            self.signals.log_message.emit(f"[发送] {text}")
            self.send_input.clear()
            
        except Exception as e:
            self.signals.log_message.emit(f"[错误] 发送失败: {str(e)}")
            
    def quick_send(self, message):
        """快捷发送"""
        if not self.client_socket:
            self.signals.log_message.emit("[错误] 没有连接的客户端")
            return
            
        try:
            data = message.encode('utf-8')
            self.client_socket.sendall(data)
            self.signals.log_message.emit(f"[发送] {message}")
        except Exception as e:
            self.signals.log_message.emit(f"[错误] 发送失败: {str(e)}")
            
    def append_log(self, message):
        """添加日志"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
        self.log_text.moveCursor(QTextCursor.End)
        
    def append_received_data(self, data):
        """添加接收的数据"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.recv_text.append(f"[{timestamp}] {data}")
        self.recv_text.moveCursor(QTextCursor.End)
        
    def on_client_connected(self, address):
        """客户端连接事件"""
        self.signals.log_message.emit(f"[连接] 客户端已连接: {address}")
        self.send_btn.setEnabled(True)
        
    def on_client_disconnected(self, _):
        """客户端断开事件"""
        self.signals.log_message.emit(f"[断开] 客户端已断开")
        self.send_btn.setEnabled(False)
        
    def closeEvent(self, event):
        """窗口关闭事件"""
        self.stop_server()
        event.accept()


def main():
    app = QApplication(sys.argv)
    server = TCPServer()
    server.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
