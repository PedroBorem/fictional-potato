import subprocess
import tkinter as tk
from tkinter import filedialog
import os

def gravar_firmware(porta_entry):
    porta = porta_entry.get()
    firmware_path = "v2.0.0"  # Aqui é onde você define a versão do firmware
    esptool_path = os.path.join(os.getcwd(), "esptool", "esptool.py")
    
    if verificar_porta(porta):
        comando = f"python {esptool_path} --chip esp32s3 -p {porta} -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 {firmware_path}/bootloader.bin 0x10000 {firmware_path}/fw_PlacaDeControle_2.bin 0x8000 {firmware_path}/partition-table.bin"
        subprocess.run(comando, shell=True)
    else:
        tk.messagebox.showerror("Erro", "Porta inválida!")

def verificar_porta(porta):
    if porta.startswith("COM") or porta.startswith("/dev/ttyUSB") or porta.startswith("/dev/ttyACM"):
        return True
    else:
        return False

def criar_interface():
    root = tk.Tk()
    root.title("Gravação do ESP32")

    logo = tk.PhotoImage(file="logo.png")
    logo_label = tk.Label(root, image=logo)
    logo_label.pack()

    firmware_label = tk.Label(root, text="Versão de Firmware: v2.0.0")  # Texto informativo da versão do firmware
    firmware_label.pack()

    porta_label = tk.Label(root, text="Porta:")
    porta_label.pack()

    porta_entry = tk.Entry(root)
    porta_entry.pack()

    gravar_button = tk.Button(root, text="Gravar", bg="red", fg="white", font=("Arial", 16), command=lambda: gravar_firmware(porta_entry))
    gravar_button.pack(pady=20)

    root.mainloop()

if __name__ == "__main__":
    criar_interface()
