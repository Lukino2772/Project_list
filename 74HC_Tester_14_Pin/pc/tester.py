import time
import tkinter as tk
from tkinter import ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    from tkinter import messagebox
    root = tk.Tk()
    root.withdraw()
    messagebox.showerror("74HC Tester", "Install pyserial with: py -m pip install pyserial")
    root.destroy()
    raise SystemExit(1)


class Tester:
    def __init__(self, root):
        self.root = root
        self.link = None
        self.buffer = bytearray()
        self.discard = False
        self.last_reply = self.last_query = 0.0
        self.pending = False
        self.started_at = None
        self.verified = False
        root.title("74HC Logic Tester")
        root.resizable(False, False)
        panel = ttk.Frame(root, padding=24)
        panel.grid()
        ttk.Label(panel, text="74HC Logic Tester", font=("Segoe UI", 19, "bold")).grid(
            row=0, column=0, columnspan=3, sticky="w", pady=(0, 16))
        self.port = ttk.Combobox(panel, width=16, state="readonly")
        self.port.grid(row=1, column=0, sticky="ew")
        ttk.Button(panel, text="Refresh", command=self.refresh).grid(row=1, column=1, padx=6)
        self.connect = ttk.Button(panel, text="Connect", command=self.toggle)
        self.connect.grid(row=1, column=2, sticky="ew")
        self.connection = tk.StringVar(value="Disconnected")
        ttk.Label(panel, textvariable=self.connection, wraplength=350).grid(
            row=2, column=0, columnspan=3, sticky="w", pady=(8, 16))
        self.heading = tk.StringVar(value="Ready when connected")
        self.detail = tk.StringVar(value="Insert a compatible 14-pin chip.")
        ttk.Label(panel, textvariable=self.heading, font=("Segoe UI", 15, "bold")).grid(
            row=3, column=0, columnspan=3, sticky="w")
        ttk.Label(panel, textvariable=self.detail, wraplength=350).grid(
            row=4, column=0, columnspan=3, sticky="w", pady=(4, 14))
        self.run = ttk.Button(panel, text="Run Test", command=self.start, state="disabled")
        self.run.grid(row=5, column=0, columnspan=3, sticky="ew")
        self.refresh()
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(50, self.poll)

    def refresh(self):
        try:
            ports = [item.device for item in list_ports.comports()]
        except (OSError, serial.SerialException):
            ports = []
        selected = self.port.get()
        self.port.configure(values=ports)
        self.port.set(selected if selected in ports else (ports[0] if ports else ""))

    def disconnect(self, message="Disconnected"):
        link, self.link = self.link, None
        if link is not None:
            try:
                link.close()
            except (OSError, serial.SerialException):
                pass
        self.buffer.clear()
        self.discard = self.pending = self.verified = False
        self.started_at = None
        self.connection.set(message)
        self.connect.configure(text="Connect")
        self.run.state(["disabled"])

    def toggle(self):
        if self.link is not None:
            self.disconnect()
            return
        device = self.port.get()
        if not device:
            self.connection.set("Select a COM port.")
            return
        try:
            self.link = serial.Serial(device, 115200, timeout=0, write_timeout=0.15)
        except (OSError, serial.SerialException) as error:
            self.disconnect(str(error))
            return
        self.last_reply = time.monotonic()
        self.last_query = 0.0
        self.connect.configure(text="Disconnect")
        self.connection.set(f"Checking {device}…")

    def send(self, data):
        try:
            if self.link.write(data) != len(data):
                raise serial.SerialTimeoutException("Incomplete USB write")
            return True
        except (OSError, serial.SerialException) as error:
            self.disconnect(str(error))
            return False

    def start(self):
        if self.link is not None and self.verified and self.send(b"RUN\n"):
            self.started_at = time.monotonic()
            self.pending = True
            self.heading.set("Testing…")
            self.detail.set("Leave the chip in the socket.")
            self.run.state(["disabled"])

    def accept(self, line):
        fields = line.decode("ascii", errors="replace").split(",")
        if len(fields) != 4 or fields[0] != "HC1":
            return
        _, status, part, count = fields
        if status not in {"READY", "TESTING", "PASS", "AMBIGUOUS", "NO_MATCH"}:
            return
        self.last_reply = time.monotonic()
        self.verified = True
        if self.started_at is not None:
            if status in {"READY", "PASS", "AMBIGUOUS", "NO_MATCH"} and time.monotonic() - self.started_at < 1.0:
                return  # A STATUS reply sent immediately before RUN may still be queued.
            self.started_at = None
        self.pending = status == "TESTING"
        self.connection.set(f"Connected · {self.link.port}")
        messages = {
            "READY": ("Ready", "Insert a compatible 14-pin chip."),
            "TESTING": ("Testing…", "Leave the chip in the socket."),
            "PASS": (f"PASS · {part}", "Matching logic and pinout. You can remove the chip."),
            "AMBIGUOUS": (f"{count} matches", "Check the part marking."),
            "NO_MATCH": ("No match", "Check the chip type, orientation, and socket contacts."),
        }
        heading, detail = messages[status]
        self.heading.set(heading)
        self.detail.set(detail)
        self.run.state(["disabled" if self.pending else "!disabled"])

    def poll(self):
        if self.link is not None:
            try:
                data = self.link.read(512)
                for byte in data:
                    if byte == 10:
                        if not self.discard:
                            self.accept(bytes(self.buffer).rstrip(b"\r"))
                        self.buffer.clear()
                        self.discard = False
                    elif len(self.buffer) >= 96:
                        self.discard = True
                    elif not self.discard:
                        self.buffer.append(byte)
                now = time.monotonic()
                if now - self.last_reply > (12 if self.verified else 4):
                    self.disconnect("Tester did not respond. Check the COM port and reconnect.")
                elif now - self.last_query >= 0.5:
                    self.last_query = now
                    self.send(b"STATUS\n")
            except (OSError, serial.SerialException) as error:
                self.disconnect(str(error))
        self.root.after(50, self.poll)

    def close(self):
        self.disconnect()
        self.root.destroy()


if __name__ == "__main__":
    window = tk.Tk()
    Tester(window)
    window.mainloop()
