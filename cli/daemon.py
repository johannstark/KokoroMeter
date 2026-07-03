"""Daemon logic for KokoroMeter.

This module handles scanning for the ESP32 serial port, fetching data
from antigravity-usage, and communicating over serial.
"""

import json
import subprocess
import sys
import threading
import time

import serial
import serial.tools.list_ports
from rich.console import Console

console = Console()


def find_serial_port() -> str | None:
    """Attempt to automatically find an ESP32 or similar serial port.

    Returns:
        str | None: The device path if found, None otherwise.
    """
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if any(keyword in p.description for keyword in ["USB", "UART", "CH340", "CP210"]):
            return p.device
    if ports:
        return ports[0].device
    return None


def fetch_quota() -> dict | None:
    """Fetch the quota from antigravity-usage via npx.

    Returns:
        dict | None: The parsed JSON data of the quota, or None on error.
    """
    try:
        result = subprocess.run(
            ["npx", "antigravity-usage", "quota", "--json"],
            capture_output=True,
            text=True,
            check=True,
        )
        return json.loads(result.stdout)
    except subprocess.CalledProcessError as e:
        console.print(
            f"[red]Error fetching quota. Make sure antigravity-usage is configured: {e}[/red]"
        )
        return None
    except json.JSONDecodeError:
        console.print("[red]Failed to parse JSON from antigravity-usage output.[/red]")
        return None


def read_from_serial(ser: serial.Serial) -> None:
    """Continuously read from the serial port for incoming actions.

    Args:
        ser (serial.Serial): The active serial connection.
    """
    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode("utf-8").strip()
                if line:
                    console.print(f"[blue][ESP32]: {line}[/blue]")
                    if line == "ACTION_REFRESH":
                        console.print("[yellow]Manual refresh requested from screen![/yellow]")
                        send_quota_update(ser)
        except Exception as e:
            console.print(f"[red]Serial read error: {e}[/red]")
            break
        time.sleep(0.1)


def send_quota_update(ser: serial.Serial) -> None:
    """Fetch and send the latest quota to the ESP32.

    Args:
        ser (serial.Serial): The active serial connection.
    """
    quota_data = fetch_quota()
    if quota_data:
        payload = json.dumps({"type": "quota", "data": quota_data})
        ser.write((payload + "\n").encode("utf-8"))
        console.print("[green]Sent quota update to ESP32.[/green]")


def run_daemon() -> None:
    """Start the daemon polling loop and serial listener.

    This function blocks forever until terminated.
    """
    console.print("[bold green]Starting KokoroMeter Daemon...[/bold green]")
    port = find_serial_port()
    if not port:
        console.print("[bold red]No serial port found. Please connect your ESP32.[/bold red]")
        sys.exit(1)

    console.print(f"[cyan]Connecting to ESP32 on {port}...[/cyan]")
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for ESP32 to reset after connection
    except Exception as e:
        console.print(f"[bold red]Failed to open port {port}: {e}[/bold red]")
        sys.exit(1)

    listener = threading.Thread(target=read_from_serial, args=(ser,), daemon=True)
    listener.start()

    poll_interval = 60  # seconds

    try:
        while True:
            send_quota_update(ser)
            time.sleep(poll_interval)
    except KeyboardInterrupt:
        console.print("\n[yellow]Shutting down daemon...[/yellow]")
        sys.exit(0)
