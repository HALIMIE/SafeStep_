#!/usr/bin/env python3
import socket
import threading
import sys
import time
import logging
import cv2
import os
import random
import numpy as np
from ultralytics import YOLO 

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

BUF_SIZE = 256
NAME_SIZE = 20
CLIENT_ID = "JETSON"
SERVER_IP = "10.10.141.80" 
RECONNECT_DELAY = 5
MAX_RECONNECT_ATTEMPTS = 0

NFS_IMAGE_PATH = "/mnt/pi_nfs/Pictures/"
SQL_IMAGE_PATH = "images/"

YOLO_MODEL_PATH = "./best.pt" 

class IoTClient:
    def __init__(self, server_ip, port, client_id):
        self.server_ip = server_ip
        self.port = port
        self.client_id = client_id
        self.sock = None
        self.connected = False
        self.running = True
        self.reconnect_count = 0
        self.rcv_thread = None
        self.reconnect_thread = None

        try:
            self.model = YOLO(YOLO_MODEL_PATH)
        except Exception as e:
            self.model = None

    def connect(self):
        try:
            if self.sock:
                self.sock.close()

            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.server_ip, self.port))

            id_packet = f"[{self.client_id}]"
            self.sock.send(id_packet.encode('utf-8'))

            self.sock.settimeout(1)
            try:
                response = self.sock.recv(BUF_SIZE).decode('utf-8')
                if "등록되지 않은 ID" in response:
                    self.sock.close()
                    return False
            except socket.timeout:
                logger.warning("서버에서 응답이 없습니다. 연결은 계속 유지합니다.")

            self.sock.settimeout(None)
            self.connected = True
            self.reconnect_count = 0
            return True

        except Exception as e:
            self.connected = False
            return False

    def start(self):
        if self.connect():
            self.rcv_thread = threading.Thread(target=self.recv_msg)
            self.rcv_thread.daemon = True
            self.rcv_thread.start()
            return True
        else:
            self.start_reconnect()
            return False

    def start_reconnect(self):
        if not self.reconnect_thread or not self.reconnect_thread.is_alive():
            self.reconnect_thread = threading.Thread(target=self.reconnect)
            self.reconnect_thread.daemon = True
            self.reconnect_thread.start()

    def reconnect(self):
        while self.running and (MAX_RECONNECT_ATTEMPTS == 0 or self.reconnect_count < MAX_RECONNECT_ATTEMPTS):
            if not self.connected:
                self.reconnect_count += 1
                if self.connect():
                    self.rcv_thread = threading.Thread(target=self.recv_msg)
                    self.rcv_thread.daemon = True
                    self.rcv_thread.start()
                    return
                time.sleep(RECONNECT_DELAY)
            else:
                return

    def recv_msg(self):
        while self.running and self.connected:
            try:
                data = self.sock.recv(BUF_SIZE + NAME_SIZE)
                if not data:
                    self.connected = False
                    self.start_reconnect()
                    break

                decoded_msg = data.decode('utf-8').strip()

                if decoded_msg == "[HX_ARD] 1":
                    original_image_path, grayscale_image_path = self.capture_image()
                    if original_image_path:
                        wear_percentage = self.analyze_wear(original_image_path)
                        self.send_data_to_server_sql(original_image_path, wear_percentage)
                else:
                    print(decoded_msg, end='')

            except Exception as e:
                if self.running:
                    self.connected = False
                    self.start_reconnect()
                break

    def capture_image(self):
        try:
            if not os.path.exists(NFS_IMAGE_PATH):
                os.makedirs(NFS_IMAGE_PATH)

            timestamp = int(time.time())
            filename = f"captured_{timestamp}.jpg"
            gray_filename = f"captured_{timestamp}_gray.jpg"

            original_image_path = os.path.join(NFS_IMAGE_PATH, filename)
            grayscale_image_path = os.path.join(NFS_IMAGE_PATH, gray_filename)
            sql_image_path = os.path.join(SQL_IMAGE_PATH, filename)

            cap = cv2.VideoCapture(0)
            if not cap.isOpened():
                return None, None

            ret, frame = cap.read()
            cap.release()

            if ret:
                cv2.imwrite(original_image_path, frame)

                gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                cv2.imwrite(grayscale_image_path, gray_frame)

                return sql_image_path, grayscale_image_path
            else:
                return None, None

        except Exception as e:
            return None, None

    def analyze_wear(self, image_path):
        try:
            if self.model is None:
                return round(random.uniform(75.0, 100.0), 0)

            actual_image_path = image_path.replace(SQL_IMAGE_PATH, NFS_IMAGE_PATH)
            if not os.path.exists(actual_image_path):
                return round(random.uniform(75.0, 100.0), 0)

            results = self.model(actual_image_path)

            if len(results) > 0:
                result = results[0]
                max_wear = 0

                for box in result.boxes:
                    cls = int(box.cls)
                    conf = float(box.conf)

                    label = self.model.names[cls]

                    if conf < 0.5:
                        continue

                    if 'normal' in label.lower():
                        wear = 0
                    elif '_' in label:
                        try:
                            wear = int(label.split('_')[1])
                        except:
                            wear = 0
                    else:
                        wear = 0

                    if wear > max_wear:
                        max_wear = wear

                if max_wear == 0:
                    return 10.0

                return float(max_wear)
            else:
                return 10.0

        except Exception as e:
            return round(random.uniform(10.0, 40.0), 0)

    def send_data_to_server_sql(self, image_path, wear_percentage):
        if not self.connected:
            return False

        try:
            timestamp = int(time.time())
            sql_message = f"[SERVER_SQL]:image_path:{image_path} wear_percentage:{wear_percentage} timestamp:{timestamp}\n"
            self.sock.send(sql_message.encode('utf-8'))
            return True
        except Exception as e:
            self.connected = False
            self.start_reconnect()
            return False

    def run(self):
        try:
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            self.running = False
        finally:
            if self.sock:
                self.sock.close()

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) == 2 else 5000
    client = IoTClient(SERVER_IP, port, CLIENT_ID)
    client.run()
