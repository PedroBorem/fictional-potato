import subprocess
import tkinter as tk
from tkinter import messagebox
import os
import platform
from serial.tools import list_ports

ROOT = "./flash"

def gravar_firmware(porta_var):
    porta = porta_var.get()
    if platform.system() == "Linux":
        firmware_path = f"v2.0.0"  # Aqui é onde você define a versão do firmware
        esptool_path = os.path.join(os.getcwd(), "esptool", "esptool.py")
    else:
        firmware_path = f"{ROOT}/v2.0.0"  # Aqui é onde você define a versão do firmware
        esptool_path = f"{ROOT}/esptool/esptool.py"

    if verificar_porta(porta):
        comando = f"python {esptool_path} --chip esp32s3 -p {porta} -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0 {firmware_path}/bootloader.bin 0x10000 {firmware_path}/fw_PlacaDeControle_2.bin 0x8000 {firmware_path}/partition-table.bin"

        try:
            subprocess.run(comando, shell=True, check=True)
            messagebox.showinfo("Sucesso", "Firmware gravado com sucesso!")
        except subprocess.CalledProcessError:
            messagebox.showerror("Erro", "Erro ao gravar o firmware.")
    else:
        messagebox.showerror("Erro", "Porta inválida!")

def verificar_porta(porta):
    if porta.startswith("COM") or porta.startswith("/dev/ttyUSB") or porta.startswith("/dev/ttyACM"):
        return True
    else:
        return False

def criar_interface():
    def get_available_ports():
        ports = [port.device for port in list_ports.comports()]
        return ports

    def update_port_list():
        available_ports = get_available_ports()
        porta_menu['menu'].delete(0, 'end')  # Clear existing menu items
        for port in available_ports:
            porta_menu['menu'].add_command(label=port, command=tk._setit(porta_var, port))

    root = tk.Tk()
    root.title("Gravação do ESP32")

    if platform.system() == "Linux":
        logo = tk.PhotoImage(file=f"logo.png")
    else:
        logo = tk.PhotoImage(file=f"{ROOT}/logo.png")

    logo_label = tk.Label(root, image=logo)
    logo_label.pack()

    firmware_label = tk.Label(root, text="Versão de Firmware: v2.0.0")  # Texto informativo da versão do firmware
    firmware_label.pack()

    # Create a Label and a OptionMenu (drop-down menu) to display available ports
    porta_label = tk.Label(root, text="Porta:")
    porta_label.pack()

    porta_var = tk.StringVar(root)
    porta_menu = tk.OptionMenu(root, porta_var, *get_available_ports())
    porta_menu.pack()

    # Create a button to update the list of available ports
    update_button = tk.Button(root, text="Atualizar lista de Portas", command=update_port_list)
    update_button.pack()

    gravar_button = tk.Button(root, text="Gravar", bg="red", fg="white", font=("Arial", 16), command=lambda: gravar_firmware(porta_var))
    gravar_button.pack(pady=20)

    root.mainloop()

if __name__ == "__main__":
    criar_interface()
