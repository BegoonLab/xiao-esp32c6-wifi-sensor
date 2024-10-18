import os
import time
import subprocess

DEVICE_PATH = "/dev/ttyACM0"
CHECK_INTERVAL = 1  # Time in seconds between checks


def send_notification(message):
    """Send a notification using notify-send."""
    subprocess.run(['notify-send', 'Device Status', message], check=True)


def monitor_device():
    """Monitor the presence of the specified device."""
    device_connected = False
    while True:
        device_exists = os.path.exists(DEVICE_PATH)

        if device_exists and not device_connected:
            send_notification(f"{DEVICE_PATH} is now available")
            device_connected = True
        elif not device_exists and device_connected:
            send_notification(f"{DEVICE_PATH} is now disconnected")
            device_connected = False

        time.sleep(CHECK_INTERVAL)


if __name__ == "__main__":
    monitor_device()
