@echo off
setlocal EnableExtensions

set ROOT=%~dp0..
set OUT=%~dp0sim_ecg.exe
set INC=-I"%ROOT%\Core\Inc"
set SRC=%ROOT%\Core\Src\ecg_filter.c ^
        %ROOT%\Core\Src\r_peak.c ^
        %ROOT%\Core\Src\heart_rate.c ^
        %ROOT%\Core\Src\signal_quality.c ^
        %ROOT%\Core\Src\uart_protocol.c ^
        %ROOT%\Core\Src\ecg_pipeline.c ^
        %~dp0sim_main.c

set "GCC=gcc"

where gcc >nul 2>&1
if errorlevel 1 (
    if defined GCC_PATH (
        set "GCC=%GCC_PATH%"
    ) else if exist "C:\Users\sys102.QNEUROIN\msys64\ucrt64\bin\gcc.exe" (
        set "GCC=C:\Users\sys102.QNEUROIN\msys64\ucrt64\bin\gcc.exe"
    ) else if exist "C:\msys64\ucrt64\bin\gcc.exe" (
        set "GCC=C:\msys64\ucrt64\bin\gcc.exe"
    ) else if exist "%USERPROFILE%\msys64\ucrt64\bin\gcc.exe" (
        set "GCC=%USERPROFILE%\msys64\ucrt64\bin\gcc.exe"
    ) else (
        echo ERROR: gcc not found.
        echo.
        echo Option 1 - use MSYS2 UCRT64 terminal and run:
        echo   cd /c/Users/sys102.QNEUROIN/STM32CubeIDE/workspace_1.18.1/ECG/host
        echo   ./build.sh
        echo.
        echo Option 2 - add MSYS2 to PATH for this CMD window:
        echo   set PATH=C:\Users\sys102.QNEUROIN\msys64\ucrt64\bin;%%PATH%%
        echo   build.bat
        echo.
        echo Option 3 - install the compiler in MSYS2:
        echo   pacman -S mingw-w64-ucrt-x86_64-gcc
        exit /b 1
    )
)

echo Using compiler: %GCC%
"%GCC%" -std=c11 -O2 -DECG_HOST_BUILD %INC% %SRC% -o "%OUT%"
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Built: %OUT%
