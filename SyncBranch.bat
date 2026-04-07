@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul

REM ===============================
REM Git 동기화 스크립트
REM (배치 파일 위치 기준 실행)
REM ===============================
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "SUBTREE_FILE=%SCRIPT_DIR%SubTreeList.txt"

git --version >nul 2>&1
if errorlevel 1 (
    echo.
    echo [ERROR] Git이 설치되어 있지 않거나 PATH에 등록되어 있지 않습니다.
    pause
    exit /b 1
)

echo.
echo ==================================================
echo                Git 동기화 시작
echo ==================================================
echo.

echo [INFO] 원격 저장소 정보를 가져오는 중...
git fetch --all --progress --verbose
if errorlevel 1 (
    echo [ERROR] git fetch 실행에 실패했습니다.
    goto FAIL_EXIT
)

REM 현재 브랜치 확인
for /f "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "CUR_BRANCH=%%i"

if "%CUR_BRANCH%"=="" (
    echo [ERROR] 현재 브랜치 이름을 확인할 수 없습니다.
    goto FAIL_EXIT
)

echo [INFO] 현재 브랜치: %CUR_BRANCH%

REM 로컬 / 원격 SHA 확인
set "LOCAL_SHA="
set "REMOTE_SHA="

for /f "tokens=*" %%i in ('git rev-parse HEAD 2^>nul') do set "LOCAL_SHA=%%i"
for /f "tokens=*" %%i in ('git rev-parse origin/%CUR_BRANCH% 2^>nul') do set "REMOTE_SHA=%%i"

if "%LOCAL_SHA%"=="" (
    echo [ERROR] 로컬 SHA를 확인할 수 없습니다.
    goto FAIL_EXIT
)

if "%REMOTE_SHA%"=="" (
    echo [WARN] origin/%CUR_BRANCH% 브랜치를 찾을 수 없습니다.
    goto FAIL_EXIT
)

if "%LOCAL_SHA%"=="%REMOTE_SHA%" (
    echo [INFO] 이미 최신 상태입니다.
    goto GIT_UPDATE_SUCCESS
)

REM 충돌 여부 확인
set "HAS_CONFLICT=0"
for /f %%A in ('git diff --name-only --diff-filter=U') do (
    set "HAS_CONFLICT=1"
)

if "!HAS_CONFLICT!"=="1" (
    echo [WARN] 현재 충돌 상태의 파일이 존재합니다.
    goto FAIL_EXIT
)

echo [INFO] 변경 사항을 가져오는 중...
git pull --no-rebase origin %CUR_BRANCH%
if errorlevel 1 (
    echo [ERROR] git pull 실행에 실패했습니다.
    goto FAIL_EXIT
)

:GIT_UPDATE_SUCCESS
echo.
echo [OK] Git 동기화가 완료되었습니다.

REM ===============================
REM Subtree 동기화
REM ===============================
echo.
echo ==================================================
echo              Subtree 동기화 시작
echo ==================================================
echo.

if not exist "%SUBTREE_FILE%" (
    echo [INFO] SubTreeList.txt 파일이 없어 Subtree 동기화를 건너뜁니다.
    goto SUBTREE_SUCCESS
)

echo [INFO] 설정 파일 읽는 중: %SUBTREE_FILE%
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
                echo [ERROR] 저장소 정보가 누락되었습니다. Prefix: !LINE_PREFIX!
                goto FAIL_EXIT
            )

            if "!LINE_BRANCH!"=="" (
                echo [ERROR] 브랜치 정보가 누락되었습니다. Prefix: !LINE_PREFIX!
                goto FAIL_EXIT
            )

            call :SYNC_SUBTREE "!LINE_PREFIX!" "!LINE_REPO!" "!LINE_BRANCH!"
            if errorlevel 1 goto FAIL_EXIT
        )
    )
)

if "!SUBTREE_FOUND!"=="0" (
    echo [INFO] 동기화할 유효한 Subtree 항목이 없습니다.
)

:SUBTREE_SUCCESS
echo.
echo [OK] Subtree 동기화가 완료되었습니다.

REM ===============================
REM Submodule 동기화
REM ===============================
echo.
echo ==================================================
echo             Submodule 동기화 시작
echo ==================================================
echo.

if not exist ".gitmodules" (
    echo [INFO] .gitmodules 파일이 없어 Submodule 동기화를 건너뜁니다.
    goto SUBMODULE_SUCCESS
)

echo [INFO] Submodule 경로 동기화 중...
git submodule sync --recursive
if errorlevel 1 (
    echo [ERROR] git submodule sync 실행에 실패했습니다.
    goto FAIL_EXIT
)

echo [INFO] Submodule 초기화 및 업데이트 중...
git submodule update --init --recursive --progress
if errorlevel 1 (
    echo [ERROR] git submodule update --init 실행에 실패했습니다.
    goto FAIL_EXIT
)

echo [INFO] Submodule 원격 업데이트 적용 중...
git submodule update --remote --recursive --progress
if errorlevel 1 (
    echo [ERROR] git submodule update --remote 실행에 실패했습니다.
    goto FAIL_EXIT
)

:SUBMODULE_SUCCESS
echo.
echo [OK] Submodule 동기화가 완료되었습니다.
goto SUCCESS_EXIT


REM ===============================
REM 종료 처리
REM ===============================

:SUCCESS_EXIT
echo.
echo ==================================================
echo                전체 작업 성공
echo ==================================================
timeout /t 3 /nobreak >nul
endlocal
exit /b 0

:FAIL_EXIT
echo.
echo ==================================================
echo                작업 실패
echo ==================================================
echo [INFO] 위 로그를 확인하여 실패 원인을 점검해주세요.
pause
endlocal
exit /b 1


REM ===============================
REM Subtree 동기화 함수
REM ===============================

:SYNC_SUBTREE
set "CURRENT_PREFIX=%~1"
set "CURRENT_REPO=%~2"
set "CURRENT_BRANCH=%~3"

echo --------------------------------------------------
echo [INFO] Subtree 확인 중
echo         경로   : %CURRENT_PREFIX%
echo         저장소 : %CURRENT_REPO%
echo         브랜치 : %CURRENT_BRANCH%
echo --------------------------------------------------

set "REMOTE_SHA="
for /f "tokens=1" %%R in ('git ls-remote --heads "%CURRENT_REPO%" "%CURRENT_BRANCH%" 2^>nul') do (
    if not defined REMOTE_SHA set "REMOTE_SHA=%%R"
)

if not defined REMOTE_SHA (
    echo [ERROR] 원격 HEAD를 확인할 수 없습니다.
    echo         저장소 : %CURRENT_REPO%
    echo         브랜치 : %CURRENT_BRANCH%
    exit /b 1
)

set "LAST_SPLIT="
for /f "tokens=2" %%S in ('git log --grep="git-subtree-dir: %CURRENT_PREFIX%" -n 50 --format^=%%B ^| findstr /b /c:"git-subtree-split:"') do (
    if not defined LAST_SPLIT set "LAST_SPLIT=%%S"
)

if not defined LAST_SPLIT (
    echo [WARN] 이전 git-subtree-split 기록을 찾지 못했습니다.
    echo [INFO] 동기화 상태를 비교할 수 없어 subtree pull을 실행합니다.

    git subtree pull --prefix="%CURRENT_PREFIX%" "%CURRENT_REPO%" "%CURRENT_BRANCH%" --squash -m "Merge From %CURRENT_PREFIX%"
    if errorlevel 1 (
        echo [ERROR] subtree pull 실행에 실패했습니다. 경로: %CURRENT_PREFIX%
        exit /b 1
    )

    echo [OK] 완료: %CURRENT_PREFIX%
    echo.
    exit /b 0
)

echo [INFO] 마지막 Split : %LAST_SPLIT%
echo [INFO] 원격 HEAD    : %REMOTE_SHA%

if /I "%LAST_SPLIT%"=="%REMOTE_SHA%" (
    echo [INFO] 원격 변경 사항이 없어 subtree pull을 생략합니다.
    echo.
    exit /b 0
)

echo [INFO] 원격 변경 사항이 감지되어 subtree pull을 실행합니다...

git subtree pull --prefix="%CURRENT_PREFIX%" "%CURRENT_REPO%" "%CURRENT_BRANCH%" --squash -m "Merge From %CURRENT_PREFIX%"
if errorlevel 1 (
    echo [ERROR] subtree pull 실행에 실패했습니다. 경로: %CURRENT_PREFIX%
    exit /b 1
)

echo [OK] 완료: %CURRENT_PREFIX%
echo.
exit /b 0