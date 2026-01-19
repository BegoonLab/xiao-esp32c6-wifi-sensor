import glob
import platform
import subprocess
import time
from datetime import datetime

CHECK_INTERVAL = 1  # Time in seconds between checks

# Device patterns by platform
# Linux: /dev/ttyACM* or /dev/ttyUSB*
# macOS: /dev/cu.* (e.g., /dev/cu.usbmodem*, /dev/cu.usbserial*, /dev/cu.SLAB_USBtoUART)
DEVICE_PATTERNS = {
    "Linux": ["/dev/ttyACM*", "/dev/ttyUSB*"],
    "Darwin": ["/dev/cu.usbmodem*", "/dev/cu.usbserial*", "/dev/cu.SLAB_USBtoUART*"],
}


def get_device_patterns():
    """Get device patterns for the current platform."""
    system = platform.system()
    return DEVICE_PATTERNS.get(system, [])


def find_devices():
    """Find all matching devices for the current platform."""
    devices = set()
    for pattern in get_device_patterns():
        devices.update(glob.glob(pattern))
    return devices


def send_notification(title, message):
    """Send a desktop notification (cross-platform)."""
    system = platform.system()
    try:
        if system == "Darwin":
            # Try terminal-notifier first (brew install terminal-notifier)
            try:
                subprocess.run(
                    [
                        "terminal-notifier",
                        "-title",
                        title,
                        "-message",
                        message,
                        "-sound",
                        "default",
                    ],
                    check=True,
                    capture_output=True,
                )
                return
            except FileNotFoundError:
                pass  # terminal-notifier not installed, try osascript

            # Fallback to osascript
            subprocess.run(
                [
                    "osascript",
                    "-e",
                    f'display notification "{message}" with title "{title}" sound name "Glass"',
                ],
                check=True,
                capture_output=True,
            )
        elif system == "Linux":
            subprocess.run(["notify-send", title, message], check=True)
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"[{timestamp()}] Notification failed: {e}")


def format_duration(seconds):
    """Format duration in human-readable format."""
    if seconds < 60:
        return f"{seconds:.1f}s"
    elif seconds < 3600:
        minutes = seconds / 60
        return f"{minutes:.1f}m"
    else:
        hours = seconds / 3600
        return f"{hours:.1f}h"


def timestamp():
    """Get current timestamp string."""
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def monitor_device():
    """Monitor the presence of serial devices."""
    system = platform.system()
    patterns = get_device_patterns()

    if not patterns:
        print(f"Unsupported platform: {system}")
        return

    print(f"[{timestamp()}] Device monitor started on {system}")
    print(f"[{timestamp()}] Watching patterns: {patterns}")

    if system == "Darwin":
        # Check if terminal-notifier is available
        try:
            subprocess.run(
                ["which", "terminal-notifier"], check=True, capture_output=True
            )
            print(f"[{timestamp()}] Notifications: using terminal-notifier")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print(
                f"[{timestamp()}] Notifications: using osascript (install terminal-notifier for better notifications: brew install terminal-notifier)"
            )
            print(
                f"[{timestamp()}] Tip: Check System Settings → Notifications → Script Editor if notifications don't appear"
            )

    # Track connected devices and their connection times
    connected_devices = {}  # device -> connection_timestamp

    # Check for already connected devices at startup
    initial_devices = find_devices()
    if initial_devices:
        print(
            f"[{timestamp()}] Found {len(initial_devices)} device(s) already connected:"
        )
        for device in initial_devices:
            connected_devices[device] = time.time()
            print(f"  - {device}")
    else:
        print(f"[{timestamp()}] No devices currently connected. Waiting...")

    while True:
        current_devices = find_devices()

        # Check for new devices
        new_devices = current_devices - set(connected_devices.keys())
        for device in new_devices:
            connected_devices[device] = time.time()
            msg = f"{device} connected"
            print(f"[{timestamp()}] ✓ {msg} (total: {len(connected_devices)})")
            send_notification("Device Connected", msg)

        # Check for disconnected devices
        removed_devices = set(connected_devices.keys()) - current_devices
        for device in removed_devices:
            connection_time = connected_devices.pop(device)
            duration = time.time() - connection_time
            msg = f"{device} disconnected"
            print(
                f"[{timestamp()}] ✗ {msg} (was connected for {format_duration(duration)}, remaining: {len(connected_devices)})"
            )
            send_notification("Device Disconnected", msg)

        time.sleep(CHECK_INTERVAL)


if __name__ == "__main__":
    monitor_device()
