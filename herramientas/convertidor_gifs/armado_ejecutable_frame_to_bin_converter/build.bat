@echo off
echo ============================================================
echo  Compilando convertir_gifs_gui.py  a  convertir_gifs.exe
echo ============================================================

python -m PyInstaller --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] PyInstaller no encontrado. Instalalo con:
    echo         pip install pyinstaller
    pause
    exit /b 1
)

python -m PyInstaller ^
    --onefile ^
    --windowed ^
    --name convertir_gifs ^
    --distpath . ^
    --workpath build_temp ^
    --specpath build_temp ^
    convertir_gifs_gui.py

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] La compilacion fallo.
    pause
    exit /b 1
)

echo.
echo [OK] Ejecutable generado: convertir_gifs.exe
echo      Coloca convertir_gifs.exe junto a gif.exe en esta carpeta.
echo.
pause
