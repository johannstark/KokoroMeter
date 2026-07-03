"""CLI application for KokoroMeter using Typer.

Provides commands to setup firmware and manage the background daemon.
"""

import os
import subprocess
import sys
from pathlib import Path

import typer
from rich.console import Console

from cli.daemon import run_daemon

app = typer.Typer(help="KokoroMeter Hardware Dashboard CLI")
console = Console()

PID_FILE = Path.home() / ".kokorometer" / "daemon.pid"


def get_pid() -> int | None:
    """Retrieve the background daemon PID if it exists.

    Returns:
        int | None: The PID of the daemon, or None if not found/running.
    """
    if PID_FILE.exists():
        try:
            pid = int(PID_FILE.read_text().strip())
            # Check if process is actually running (Unix specific check)
            if os.name == "posix":
                os.kill(pid, 0)
            return pid
        except (ValueError, OSError):
            # Stale PID file or process not running
            PID_FILE.unlink(missing_ok=True)
            return None
    return None


@app.command()
def setup() -> None:
    """Guided setup to compile and flash the KokoroMeter firmware."""
    console.print("[bold cyan]KokoroMeter Firmware Setup Wizard[/bold cyan]")
    console.print("Please ensure your ESP32 is plugged into a USB port.")
    typer.confirm("Ready to flash?", abort=True)

    # Locate the firmware directory (bundled inside the package)
    pkg_dir = Path(__file__).parent.parent
    firmware_dir = pkg_dir / "firmware"

    if not firmware_dir.exists():
        console.print("[bold red]Firmware directory not found in package.[/bold red]")
        raise typer.Exit(1)

    console.print("[yellow]Compiling and uploading firmware via PlatformIO...[/yellow]")

    try:
        # We assume pio is in the path or provided by pixi
        subprocess.run(["pio", "run", "-t", "upload"], cwd=firmware_dir, check=True)
        console.print("[bold green]Firmware flashed successfully![/bold green]")
    except subprocess.CalledProcessError:
        console.print(
            "[bold red]Failed to compile or flash firmware. Ensure PlatformIO is "
            "installed and the board is connected.[/bold red]"
        )
        raise typer.Exit(1)


@app.command()
def start() -> None:
    """Start the KokoroMeter daemon in the background."""
    if get_pid() is not None:
        console.print("[yellow]KokoroMeter daemon is already running![/yellow]")
        raise typer.Exit(0)

    console.print("[cyan]Starting KokoroMeter daemon in background...[/cyan]")
    PID_FILE.parent.mkdir(parents=True, exist_ok=True)

    # Re-launch ourselves with a hidden subcommand to run the actual daemon logic
    # We use subprocess.Popen to detach it
    if os.name == "posix":
        process = subprocess.Popen(
            [sys.executable, "-m", "cli.main", "_run_daemon"],
            start_new_session=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    else:
        # Windows detached process creation flag
        DETACHED_PROCESS = 0x00000008
        process = subprocess.Popen(
            [sys.executable, "-m", "cli.main", "_run_daemon"],
            creationflags=DETACHED_PROCESS,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

    PID_FILE.write_text(str(process.pid))
    console.print(f"[bold green]Daemon started with PID {process.pid}.[/bold green]")


@app.command(hidden=True)
def _run_daemon() -> None:
    """Internal command to run the daemon loop. Not meant for direct user execution."""
    run_daemon()


@app.command()
def status() -> None:
    """Check if the KokoroMeter daemon is running."""
    pid = get_pid()
    if pid is not None:
        console.print(f"[bold green]KokoroMeter daemon is running (PID: {pid}).[/bold green]")
    else:
        console.print("[yellow]KokoroMeter daemon is NOT running.[/yellow]")


@app.command()
def stop() -> None:
    """Stop the background KokoroMeter daemon."""
    pid = get_pid()
    if pid is not None:
        console.print(f"[cyan]Stopping daemon (PID: {pid})...[/cyan]")
        import signal

        try:
            os.kill(pid, signal.SIGTERM)
            console.print("[bold green]Daemon stopped.[/bold green]")
        except OSError as e:
            console.print(f"[red]Failed to stop daemon: {e}[/red]")
        finally:
            PID_FILE.unlink(missing_ok=True)
    else:
        console.print("[yellow]Daemon is not running.[/yellow]")


if __name__ == "__main__":
    app()
