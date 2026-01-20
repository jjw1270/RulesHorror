@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ===============================
REM Git update (based on batch file location)
REM ===============================
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

git --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git is not installed or not available in PATH.
    echo         Please install Git or configure environment variables, then run again.
    pause
    exit /b 1
)

echo ============================================
echo Git Sync
echo.

echo Fetching...
git fetch --all --progress --verbose
if errorlevel 1 (
    echo Failed
    echo.
    goto AFTER_GIT_UPDATE
)

REM Get current branch name
for /f "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD') do set "CUR_BRANCH=%%i"

if "%CUR_BRANCH%"=="" (
    echo.
    echo [ERROR] Unable to determine the current branch.
    echo.
    goto AFTER_GIT_UPDATE
)

REM Get local / remote commit SHA
for /f "tokens=*" %%i in ('git rev-parse HEAD') do set "LOCAL_SHA=%%i"
for /f "tokens=*" %%i in ('git rev-parse origin/%CUR_BRANCH%') do set "REMOTE_SHA=%%i"

if "%REMOTE_SHA%"=="" (
    echo.
    echo [WARN] Unable to find origin/%CUR_BRANCH%.
    echo.
    goto AFTER_GIT_UPDATE
)

REM Compare whether it is the latest version
if "%LOCAL_SHA%"=="%REMOTE_SHA%" (
    echo.
    echo [INFO] Already up to date.
    echo.
    goto GIT_UPDATE_Success
)

REM Check conflicts
set "HAS_CONFLICT=0"
for /f %%A in ('git diff --name-only --diff-filter=U') do (
    set "HAS_CONFLICT=1"
)

if "%HAS_CONFLICT%"=="1" (
    echo.
    echo [WARN] Conflicts exist.
    echo.
    goto AFTER_GIT_UPDATE
)

REM Ask whether to sync to the latest remote version
set "DO_GIT_SYNC="
set /p DO_GIT_SYNC=Do you want to run git pull? (Cancel[C]): 

if /I "%DO_GIT_SYNC%"=="C" (
    echo.
    echo Cancelled.
    goto AFTER_GIT_UPDATE
)

echo.
echo Pulling...
git pull --no-rebase origin %CUR_BRANCH%
if errorlevel 1 (
    echo.
    echo [ERROR] git pull failed. A conflict or another issue occurred.
    echo.
    goto AFTER_GIT_UPDATE
)

:GIT_UPDATE_Success
echo ***** Git Sync Success! *****

echo.
echo ============================================
echo Submodule Sync
echo.

REM Sync when .gitmodules URL changes, etc.
git submodule sync --recursive
if errorlevel 1 (
    echo.
    echo [ERROR] git submodule sync failed
    echo.
    goto AFTER_GIT_UPDATE
)

REM IMPORTANT: Initialize submodules for first-time clones, then update.
echo.
echo [INFO] Initializing submodules (first-time setup)...
git submodule update --init --recursive --progress
if errorlevel 1 (
    echo.
    echo [ERROR] submodule init/update failed
    echo.
    goto AFTER_GIT_UPDATE
)

echo.
echo [INFO] Updating submodules to remote (--remote)...
REM Note: If submodules do not have branch settings, behavior may differ from expectations.
git submodule update --remote --recursive --progress
if errorlevel 1 (
    echo.
    echo [ERROR] submodule remote update failed
    echo.
    goto AFTER_GIT_UPDATE
)

:SUBMODULE_SUCCESS
echo.
echo ***** Submodule Sync Success! *****

:AFTER_GIT_UPDATE

REM ===============================
REM Config file: store UE engine path
REM ===============================
set "CONFIG_FILE=%SCRIPT_DIR%UE_EngineDir.txt"

REM ===============================
REM 0) Load UE_ENGINE_DIR or request input
REM ===============================
if exist "%CONFIG_FILE%" (
    REM Use previously saved engine path
    set /p UE_ENGINE_DIR=<"%CONFIG_FILE%"
) else (
    set "UE_ENGINE_DIR="
)

REM Validate whether saved path exists or not
:VALIDATE_ENGINE_DIR
if not "%UE_ENGINE_DIR%"=="" (
    REM Validate by checking UnrealBuildTool.exe / Build.bat
    if exist "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" (
        if exist "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" (
            goto ENGINE_DIR_OK
        )
    )
    echo.
    echo [ERROR] The saved UE engine path is invalid.
    echo        Could not find UnrealBuildTool.exe or Build.bat.
    echo        Current path: %UE_ENGINE_DIR%
    echo.
)

REM No config or invalid config
echo ============================================
echo  UE5 Engine Directory Setup
echo  e.g.) C:\Program Files\Epic Games\UE_5.4
echo        D:\Unreal\UE_5.3
echo.

:ASK_ENGINE_DIR
set "UE_ENGINE_DIR="
set /p UE_ENGINE_DIR=Enter the UE engine folder path: 

if "%UE_ENGINE_DIR%"=="" (
    echo The engine path is empty. Please enter it again.
    echo.
    goto ASK_ENGINE_DIR
)

REM Validate UnrealBuildTool.exe / Build.bat
if not exist "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" (
    echo.
    echo [ERROR] Cannot find UnrealBuildTool.exe.
    echo Path: "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
    echo Please verify that the UE engine root directory is correct.
    echo.
    goto ASK_ENGINE_DIR
)

if not exist "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" (
    echo.
    echo [ERROR] Cannot find Build.bat.
    echo Path: "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat"
    echo Please verify that the UE engine root directory is correct.
    echo.
    goto ASK_ENGINE_DIR
)

REM Save validated engine path
> "%CONFIG_FILE%" echo %UE_ENGINE_DIR%
echo.
echo [INFO] Saved UE engine path: %UE_ENGINE_DIR%

:ENGINE_DIR_OK
echo.

REM ===============================
REM [Configuration] Project paths (AUTO DETECT)
REM ===============================
set "PROJECT_NAME="
set "PROJECT_FILE="

REM Find exactly one *.uproject next to this .bat
for %%F in ("%SCRIPT_DIR%*.uproject") do (
    if not defined PROJECT_FILE (
        set "PROJECT_FILE=%%~fF"
        set "PROJECT_NAME=%%~nF"
    ) else (
        echo.
        echo [ERROR] Multiple .uproject files found in the script folder.
        echo         Please keep only one .uproject next to this .bat.
        echo.
        echo         Found:
        echo         - !PROJECT_FILE!
        echo         - %%~fF
        pause
        exit /b 1
    )
)

if not defined PROJECT_FILE (
    echo.
    echo [ERROR] Cannot find any .uproject file in the script folder.
    echo         Folder: %SCRIPT_DIR%
    pause
    exit /b 1
)

REM UnrealBuildTool / Build.bat paths
set "GENERATE_SCRIPT=%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
set "BUILD_SCRIPT=%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat"

echo ============================================
echo  Project : %PROJECT_NAME%
echo  Engine  : %UE_ENGINE_DIR%
echo ============================================
echo.

REM ===============================
REM 1) Basic validation - uproject / UBT / Build.bat
REM ===============================
if not exist "%PROJECT_FILE%" (
    echo [ERROR] Cannot find project file.
    echo Path: %PROJECT_FILE%
    pause
    exit /b 1
)

if not exist "%GENERATE_SCRIPT%" (
    echo [ERROR] Cannot find UnrealBuildTool.exe.
    echo Path: %GENERATE_SCRIPT%
    pause
    exit /b 1
)

if not exist "%BUILD_SCRIPT%" (
    echo [ERROR] Cannot find Build.bat.
    echo Path: %BUILD_SCRIPT%
    pause
    exit /b 1
)

REM ===============================
REM 2) Generate Visual Studio project files
REM ===============================
echo Generate Visual Studio project files...
call "%GENERATE_SCRIPT%" -projectfiles -project="%PROJECT_FILE%" -game -progress
if errorlevel 1 (
    echo.
    echo ***** GenerateProjectFiles FAILED *****
    pause
    exit /b 1
)

REM ===============================
REM 3) Build Development Editor
REM ===============================
echo.
echo Build %PROJECT_NAME%Editor Win64 Development...
call "%BUILD_SCRIPT%" %PROJECT_NAME%Editor Win64 Development -project="%PROJECT_FILE%" -WaitMutex -NoHotReload -progress
if errorlevel 1 (
    echo.
    echo ***** Build FAILED *****
    pause
    exit /b 1
)

REM ===============================
REM 4) Launch editor automatically on success
REM ===============================
echo.
echo Launch Project...
start "" "%PROJECT_FILE%"

REM ===============================
REM 5) Exit after countdown
REM ===============================
timeout /t 3 /nobreak >nul

endlocal
exit /b 0
