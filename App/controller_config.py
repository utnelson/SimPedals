import serial
import tkinter as tk
from tkinter import messagebox
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
from collections import deque
import threading
import time

# ====== EINSTELLUNGEN ======
PORT = "COM15"     # <-- hier COM-Port deines Arduino eintragen!
BAUD = 115200

# ====== Globale Variablen ======
running = True
brake_values = deque(maxlen=200)     # speichert letzte 200 Werte
throttle_values = deque(maxlen=200)

# ====== Serielle Verbindung ======
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
except serial.SerialException:
    ser = None
    messagebox.showerror("Fehler", f"Kann Port {PORT} nicht öffnen.")

# ====== Tkinter-Hauptfenster ======
root = tk.Tk()
root.title("Controller Konfiguration + Live Plot")

# Eingabefeld + Buttons
tk.Label(root, text="Throttle Min:").grid(row=0, column=0, padx=5, pady=5)
entry_throttle_min = tk.Entry(root)
entry_throttle_min.grid(row=0, column=1, padx=5, pady=5)

label_status = tk.Label(root, text="Nicht verbunden" if not ser else "Verbunden")
label_status.grid(row=0, column=2, padx=5, pady=5)

# ====== Matplotlib-Plot vorbereiten ======
fig, ax = plt.subplots(figsize=(6, 3))
line_brake, = ax.plot([], [], label="Brake (Kupplung)")
line_throttle, = ax.plot([], [], label="Throttle (Gas)")
ax.set_ylim(0, 1023)
ax.set_xlim(0, 200)
ax.legend()
ax.set_xlabel("Samples")
ax.set_ylabel("Analogwert")

canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().grid(row=1, column=0, columnspan=3, padx=10, pady=10)

# ====== Funktionen ======
def send_command(cmd):
    if not ser or not ser.is_open:
        return ""
    ser.write((cmd + "\n").encode())
    return ser.readline().decode().strip()

def update_plot():
    # Update der Linien mit neuen Daten
    line_brake.set_data(range(len(brake_values)), list(brake_values))
    line_throttle.set_data(range(len(throttle_values)), list(throttle_values))
    canvas.draw()

def serial_reader():
    """Liest kontinuierlich Daten von Arduino"""
    global running
    while running and ser and ser.is_open:
        try:
            line = ser.readline().decode().strip()
            if "\t" in line:
                brake, throttle = line.split("\t")
                brake_values.append(int(brake))
                throttle_values.append(int(throttle))
        except:
            pass
        time.sleep(0.01)  # 10ms Pause

def refresh_gui():
    update_plot()
    root.after(50, refresh_gui)  # alle 50ms Plot updaten

# ====== Thread für serielle Daten starten ======
if ser:
    threading.Thread(target=serial_reader, daemon=True).start()

# ====== GUI-Update starten ======
refresh_gui()

# ====== Sauber beenden ======
def on_close():
    global running
    running = False
    if ser and ser.is_open:
        ser.close()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)

root.mainloop()
