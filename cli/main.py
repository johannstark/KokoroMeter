"""CLI application for KokoroMeter using Typer.

Provides a configuration and flashing wizard for the standalone Wi-Fi KokoroMeter.
"""

import json
import time
import subprocess
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import typer
from rich.console import Console
import serial
import serial.tools.list_ports

app = typer.Typer(help="KokoroMeter Configuration Wizard")
console = Console()


@app.command()
def version() -> None:
    """Show the current version."""
    console.print("KokoroMeter v0.1.0")


def get_gcloud_credentials() -> Tuple[str, str, str]:
    """Scan for gcloud credentials and prompt if multiple accounts exist."""
    gcloud_dir = Path.home() / ".config" / "gcloud"
    legacy_dir = gcloud_dir / "legacy_credentials"
    
    accounts = []
    if legacy_dir.exists() and legacy_dir.is_dir():
        for item in legacy_dir.iterdir():
            if item.is_dir() and (item / "adc.json").exists():
                accounts.append(item.name)
    
    target_file: Optional[Path] = None
    
    if len(accounts) > 1:
        console.print("\n[yellow]Multiple Google Cloud accounts found![/yellow]")
        for i, acc in enumerate(accounts, 1):
            console.print(f"{i}. {acc}")
        
        choice = typer.prompt("Select the account to use for KokoroMeter", type=int)
        if 1 <= choice <= len(accounts):
            target_file = legacy_dir / accounts[choice - 1] / "adc.json"
        else:
            console.print("[red]Invalid choice.[/red]")
            raise typer.Exit(1)
            
    elif len(accounts) == 1:
        console.print(f"[green]Found Google Cloud account: {accounts[0]}[/green]")
        target_file = legacy_dir / accounts[0] / "adc.json"
    
    # Fallback to the default credentials file if legacy doesn't exist or is empty
    if not target_file:
        default_file = gcloud_dir / "application_default_credentials.json"
        if default_file.exists():
            target_file = default_file
            console.print("[green]Found default Google Cloud credentials![/green]")
    
    if not target_file or not target_file.exists():
        console.print("[bold red]No Google Cloud credentials found![/bold red]")
        console.print("Please authenticate first by running: [cyan]gcloud auth application-default login[/cyan]")
        console.print("(or simply run [cyan]npx antigravity-usage auth[/cyan])")
        raise typer.Exit(1)
        
    try:
        data = json.loads(target_file.read_text())
        client_id = data.get("client_id")
        client_secret = data.get("client_secret")
        refresh_token = data.get("refresh_token")
        
        if not (client_id and client_secret and refresh_token):
            raise ValueError("Missing OAuth fields in JSON")
            
        return client_id, client_secret, refresh_token
        
    except Exception as e:
        console.print(f"[bold red]Failed to parse credentials file: {e}[/bold red]")
        raise typer.Exit(1)


def find_esp32_port() -> str:
    """Find the USB Serial port for the ESP32."""
    ports = serial.tools.list_ports.comports()
    # Typical ESP32 USB to UART bridge chips
    for port in ports:
        if "CP210" in port.description or "CH340" in port.description or "USB to UART" in port.description or "usbserial" in port.device:
            return port.device
            
    # Fallback: Let user choose
    if not ports:
        console.print("[bold red]No serial ports found![/bold red]")
        raise typer.Exit(1)
        
    console.print("\n[yellow]Select the Serial Port for your ESP32:[/yellow]")
    for i, port in enumerate(ports, 1):
        console.print(f"{i}. {port.device} - {port.description}")
        
    choice = typer.prompt("Select the port", type=int)
    if 1 <= choice <= len(ports):
        return ports[choice - 1].device
        
    console.print("[red]Invalid choice.[/red]")
    raise typer.Exit(1)


def perform_provisioning(port: str, client_id: str, client_secret: str, refresh_token: str) -> None:
    """Connect to ESP32 over serial and provision credentials."""
    console.print(f"\n[cyan]Connecting to ESP32 on {port}...[/cyan]")
    
    try:
        # 115200 baud is standard. Opening the port usually resets the ESP32.
        with serial.Serial(port, 115200, timeout=5) as ser:
            # Wait for the device to boot and enter provisioning state
            console.print("Waiting for device to enter provisioning mode...")
            
            ready = False
            start_time = time.time()
            while time.time() - start_time < 10:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    try:
                        doc = json.loads(line)
                        if doc.get("status") == "waiting_provision":
                            ready = True
                            break
                    except json.JSONDecodeError:
                        pass
                        
            if not ready:
                console.print("[bold red]Failed to detect provisioning state from device.[/bold red]")
                console.print("Make sure the firmware was flashed correctly and the device is ready.")
                raise typer.Exit(1)
                
            console.print("[green]Device is ready for provisioning![/green]")
            
            # Request Wi-Fi Scan
            console.print("\n[yellow]Scanning for 2.4GHz Wi-Fi Networks on the ESP32...[/yellow]")
            ser.write(b'{"cmd":"scan"}\n')
            
            networks = []
            start_time = time.time()
            while time.time() - start_time < 15:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    try:
                        doc = json.loads(line)
                        if "networks" in doc:
                            networks = doc["networks"]
                            break
                    except json.JSONDecodeError:
                        pass
                        
            # Ask User for Wi-Fi
            ssid = ""
            if networks:
                console.print("\nAvailable Networks:")
                for i, net in enumerate(networks, 1):
                    console.print(f"{i}. {net}")
                console.print(f"{len(networks) + 1}. [Enter manually]")
                
                choice = typer.prompt("Select your Wi-Fi network", type=int)
                if 1 <= choice <= len(networks):
                    ssid = networks[choice - 1]
            else:
                console.print("[yellow]No networks found by the device.[/yellow]")
            
            if not ssid:
                ssid = typer.prompt("Enter your Wi-Fi SSID manually")
                
            password = typer.prompt("Enter your Wi-Fi Password", hide_input=True)
            
            # Send Provision Command
            console.print("\n[cyan]Sending credentials to device...[/cyan]")
            provision_payload = {
                "cmd": "provision",
                "ssid": ssid,
                "password": password,
                "client_id": client_id,
                "client_secret": client_secret,
                "refresh_token": refresh_token
            }
            ser.write((json.dumps(provision_payload) + "\n").encode('utf-8'))
            
            # Wait for success
            success = False
            start_time = time.time()
            while time.time() - start_time < 5:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    try:
                        doc = json.loads(line)
                        if doc.get("status") == "success":
                            success = True
                            break
                    except json.JSONDecodeError:
                        pass
                        
            if success:
                console.print("[bold green]Provisioning successful! The device will now reboot.[/bold green]")
            else:
                console.print("[bold red]Provisioning failed or timed out.[/bold red]")
                
    except serial.SerialException as e:
        console.print(f"[bold red]Serial port error: {e}[/bold red]")
        raise typer.Exit(1)


@app.command()
def setup() -> None:
    """Guided setup to configure credentials and flash KokoroMeter."""
    console.print("[bold cyan]KokoroMeter Standalone Wi-Fi Setup Wizard[/bold cyan]")
    
    # --- Step 1: Google Credentials ---
    console.print("\n[yellow]Step 1: Google API Credentials[/yellow]")
    console.print("Scanning your local system for existing Google Cloud credentials...")
    client_id, client_secret, refresh_token = get_gcloud_credentials()
    console.print("[green]Credentials loaded successfully![/green]")
    
    # --- Step 2: Hardware Profile ---
    console.print("\n[yellow]Step 2: Hardware Profile[/yellow]")
    console.print("1. Hosyond 4.0 TFT (Large Touch Screen)")
    console.print("2. SSD1306 OLED (Small Screen)")
    choice_hw = typer.prompt("Select your screen", type=int, default=1)
    env = "ssd1306" if choice_hw == 2 else "hosyond"

    # --- Flashing ---
    pkg_dir = Path(__file__).parent.parent
    firmware_dir = pkg_dir / "firmware"
    
    if not firmware_dir.exists():
        console.print("[bold red]Firmware directory not found in package.[/bold red]")
        raise typer.Exit(1)
        
    console.print("\nPlease ensure your ESP32 is plugged into a USB port.")
    typer.confirm("Ready to flash?", abort=True)

    console.print(f"[yellow]Compiling and uploading generic firmware ({env}) via PlatformIO...[/yellow]")

    try:
        subprocess.run(["pio", "run", "-e", env, "-t", "upload"], cwd=firmware_dir, check=True)
        console.print("[bold green]Firmware flashed successfully![/bold green]")
    except subprocess.CalledProcessError:
        console.print(
            "[bold red]Failed to compile or flash firmware. Ensure PlatformIO is "
            "installed and the board is connected.[/bold red]"
        )
        raise typer.Exit(1)
        
    # --- Step 3: USB Provisioning ---
    console.print("\n[yellow]Step 3: Device Provisioning[/yellow]")
    time.sleep(2) # Give the device a moment to reboot after flashing
    
    port = find_esp32_port()
    perform_provisioning(port, client_id, client_secret, refresh_token)


if __name__ == "__main__":
    app()
