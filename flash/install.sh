#!/bin/bash

# Verifica se o usuário é root
if [ "$(id -u)" != "0" ]; then
  echo "Este script deve ser executado como root" 1>&2
  exit 1
fi

# Atualiza a lista de pacotes e instala o Python e o pip
apt-get update
apt-get install -y python3 python3-pip

# Instala o esptool
cd esptool
python3 setup.py install
cd ..

echo "Instalação concluída com sucesso!"