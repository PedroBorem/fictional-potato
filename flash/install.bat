@echo off

rem Verifica se o usuário possui privilégios de administrador
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Este script requer privilégios de administrador
    echo Execute-o como administrador e tente novamente
    pause
    exit /b
)

rem Instala o esptool
cd esptool
python setup.py install
cd ..

echo Instalação concluída com sucesso!
pause