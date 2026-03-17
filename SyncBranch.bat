@echo off
setlocal EnableDelayedExpansion

REM ===============================
REM Git update (based on batch file location)
REM ===============================
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "SUBTREE_FILE=%SCRIPT_DIR%SubTreeList.txt"

git --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git is not installed or not available in PATH.
    pause
    exit /b 1
)

echo ============================================
echo Git Sync
echo.

echo Fetching...
git fetch --all --progress --verbose
if errorlevel 1 (
    echo [ERROR] git fetch failed.
    goto AFTER_GIT_UPDATE
)

REM Get current branch name
for /f "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "CUR_BRANCH=%%i"

if "%CUR_BRANCH%"=="" (
    echo [ERROR] Unable to determine the current branch.
    goto AFTER_GIT_UPDATE
)

echo [INFO] Current branch: %CUR_BRANCH%

REM Get SHA
set "LOCAL_SHA="
set "REMOTE_SHA="

for /f "tokens=*" %%i in ('git rev-parse HEAD 2^>nul') do set "LOCAL_SHA=%%i"
for /f "tokens=*" %%i in ('git rev-parse origin/%CUR_BRANCH% 2^>nul') do set "REMOTE_SHA=%%i"

if "%LOCAL_SHA%"=="" (
    echo [ERROR] Unable to determine local SHA.
    goto AFTER_GIT_UPDATE
)

if "%REMOTE_SHA%"=="" (
    echo [WARN] origin/%CUR_BRANCH% not found.
    goto AFTER_GIT_UPDATE
)

if "%LOCAL_SHA%"=="%REMOTE_SHA%" (
    echo [INFO] Already up to date.
    goto GIT_UPDATE_Success
)

REM Conflict check
set "HAS_CONFLICT=0"
for /f %%A in ('git diff --name-only --diff-filter=U') do (
    set "HAS_CONFLICT=1"
)

if "!HAS_CONFLICT!"=="1" (
    echo [WARN] Conflicts exist.
    goto AFTER_GIT_UPDATE
)

REM Pull confirm
set "DO_GIT_SYNC="
set /p DO_GIT_SYNC=Do you want to run git pull? (Cancel[C]): 

if /I "%DO_GIT_SYNC%"=="C" (
    echo Cancelled.
    goto AFTER_GIT_UPDATE
)

echo Pulling...
git pull --no-rebase origin %CUR_BRANCH%
if errorlevel 1 (
    echo [ERROR] git pull failed.
    goto AFTER_GIT_UPDATE
)

:GIT_UPDATE_Success
echo.
echo ***** Git Sync Success! *****

echo.
echo ============================================
echo Submodule Sync
echo.

if not exist ".gitmodules" (
    echo [INFO] No submodules. Skipping.
    goto SUBMODULE_SUCCESS
)

git submodule sync --recursive
if errorlevel 1 (
    echo [ERROR] git submodule sync failed.
    goto AFTER_GIT_UPDATE
)

git submodule update --init --recursive --progress
if errorlevel 1 (
    echo [ERROR] git submodule init/update failed.
    goto AFTER_GIT_UPDATE
)

git submodule update --remote --recursive --progress
if errorlevel 1 (
    echo [ERROR] git submodule remote update failed.
    goto AFTER_GIT_UPDATE
)

:SUBMODULE_SUCCESS
echo ***** Submodule Sync Success! *****

echo.
echo ============================================
echo Subtree Sync
echo.

if not exist "%SUBTREE_FILE%" (
    echo [INFO] SubTreeList.txt not found. Skipping.
    goto SUBTREE_SUCCESS
)

echo [INFO] Reading: %SUBTREE_FILE%
echo.

set "SUBTREE_FOUND=0"

for /f "usebackq tokens=1,2,3" %%A in ("%SUBTREE_FILE%") do (
    set "LINE_PREFIX=%%A"
    set "LINE_REPO=%%B"
    set "LINE_BRANCH=%%C"

    if not "!LINE_PREFIX!"=="" (
        if not "!LINE_PREFIX:~0,1!"=="#" (

            set "SUBTREE_FOUND=1"

            if "!LINE_REPO!"=="" (
                echo [ERROR] Repo missing: !LINE_PREFIX!
                goto AFTER_GIT_UPDATE
            )

            if "!LINE_BRANCH!"=="" (
                echo [ERROR] Branch missing: !LINE_PREFIX!
                goto AFTER_GIT_UPDATE
            )

            call :SYNC_SUBTREE "!LINE_PREFIX!" "!LINE_REPO!" "!LINE_BRANCH!"
            if errorlevel 1 goto AFTER_GIT_UPDATE
        )
    )
)

if "!SUBTREE_FOUND!"=="0" (
    echo [INFO] No valid entries.
)

:SUBTREE_SUCCESS
echo.
echo ***** Subtree Sync Success! *****

:AFTER_GIT_UPDATE
timeout /t 3 /nobreak >nul

endlocal
exit /b 0


:SYNC_SUBTREE
set "CURRENT_PREFIX=%~1"
set "CURRENT_REPO=%~2"
set "CURRENT_BRANCH=%~3"

echo [INFO] Checking subtree
echo        Prefix : %CURRENT_PREFIX%
echo        Repo   : %CURRENT_REPO%
echo        Branch : %CURRENT_BRANCH%

set "REMOTE_SHA="
for /f "tokens=1" %%R in ('git ls-remote --heads "%CURRENT_REPO%" "%CURRENT_BRANCH%" 2^>nul') do (
    if not defined REMOTE_SHA set "REMOTE_SHA=%%R"
)

if not defined REMOTE_SHA (
    echo [ERROR] Unable to resolve remote HEAD for %CURRENT_REPO% %CURRENT_BRANCH%
    exit /b 1
)

set "LAST_SPLIT="
for /f "tokens=2" %%S in ('git log --grep="git-subtree-dir: %CURRENT_PREFIX%" -n 50 --format^=%%B ^| findstr /b /c:"git-subtree-split:"') do (
    if not defined LAST_SPLIT set "LAST_SPLIT=%%S"
)

if not defined LAST_SPLIT (
    echo [WARN] Previous git-subtree-split not found for %CURRENT_PREFIX%
    echo [INFO] Running subtree pull because sync state cannot be verified...

    git subtree pull --prefix="%CURRENT_PREFIX%" "%CURRENT_REPO%" "%CURRENT_BRANCH%" --squash -m "Merge From %CURRENT_PREFIX%"
    if errorlevel 1 (
        echo [ERROR] subtree pull failed for %CURRENT_PREFIX%
        exit /b 1
    )

    echo [OK] Done: %CURRENT_PREFIX%
    echo.
    exit /b 0
)

echo        LastSplit  : %LAST_SPLIT%
echo        RemoteHEAD : %REMOTE_SHA%

if /I "%LAST_SPLIT%"=="%REMOTE_SHA%" (
    echo [INFO] No remote changes. Skipping subtree pull.
    echo.
    exit /b 0
)

echo [INFO] Remote changes detected. Running subtree pull...

git subtree pull --prefix="%CURRENT_PREFIX%" "%CURRENT_REPO%" "%CURRENT_BRANCH%" --squash -m "Merge From %CURRENT_PREFIX%"
if errorlevel 1 (
    echo [ERROR] subtree pull failed for %CURRENT_PREFIX%
    exit /b 1
)

echo [OK] Done: %CURRENT_PREFIX%
echo.
exit /b 0