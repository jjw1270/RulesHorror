@echo off
setlocal

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
echo [INFO] Submodule update --remote...
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

timeout /t 3 /nobreak >nul

endlocal
exit /b 0
