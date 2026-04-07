@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

REM ===============================
REM 기본 설정
REM ===============================
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

set "CONFIG_FILE=%SCRIPT_DIR%UE_EngineDir.txt"
set "SUBTREE_FILE=%SCRIPT_DIR%SubTreeList.txt"

set "UE_ENGINE_DIR="
set "PROJECT_NAME="
set "PROJECT_FILE="
set "GENERATE_SCRIPT="
set "BUILD_SCRIPT="
set "CUR_BRANCH="
set "LOCAL_SHA="
set "REMOTE_SHA="

call :PRINT_HEADER "Git 동기화 + UE 프로젝트 생성/빌드"

REM ===============================
REM 1) Git 확인
REM ===============================
git --version >nul 2>&1
if errorlevel 1 (
    call :LOG_ERROR "Git이 설치되어 있지 않거나 PATH에 등록되어 있지 않습니다."
    call :LOG_ERROR "Git 설치 또는 환경 변수 설정 후 다시 실행해주세요."
    goto FAIL_EXIT
)

REM ===============================
REM 2) Git 동기화
REM ===============================
call :PRINT_HEADER "Git 동기화"

call :LOG_INFO "원격 저장소 정보를 가져오는 중..."
git fetch --all --progress --verbose
if errorlevel 1 (
    call :LOG_WARN "git fetch에 실패했습니다."
    call :LOG_WARN "Git 동기화 단계는 건너뛰고 다음 단계로 진행합니다."
    goto AFTER_GIT_UPDATE
)

for /f "tokens=*" %%i in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "CUR_BRANCH=%%i"

if not defined CUR_BRANCH (
    echo.
    call :LOG_ERROR "현재 브랜치 이름을 확인할 수 없습니다."
    goto AFTER_GIT_UPDATE
)

call :LOG_INFO "현재 브랜치: %CUR_BRANCH%"

for /f "tokens=*" %%i in ('git rev-parse HEAD 2^>nul') do set "LOCAL_SHA=%%i"
for /f "tokens=*" %%i in ('git rev-parse origin/%CUR_BRANCH% 2^>nul') do set "REMOTE_SHA=%%i"

if not defined REMOTE_SHA (
    echo.
    call :LOG_WARN "origin/%CUR_BRANCH% 브랜치를 찾을 수 없습니다."
    goto AFTER_GIT_UPDATE
)

if "%LOCAL_SHA%"=="%REMOTE_SHA%" (
    echo.
    call :LOG_INFO "이미 최신 상태입니다."
    goto GIT_UPDATE_SUCCESS
)

set "HAS_CONFLICT=0"
for /f %%A in ('git diff --name-only --diff-filter=U') do (
    set "HAS_CONFLICT=1"
)

if "!HAS_CONFLICT!"=="1" (
    echo.
    call :LOG_WARN "충돌 상태의 파일이 존재합니다."
    call :LOG_WARN "git pull을 수행하지 않고 다음 단계로 진행합니다."
    goto AFTER_GIT_UPDATE
)

set "DO_GIT_SYNC="
echo.
set /p DO_GIT_SYNC=원격 변경 사항이 있습니다. git pull을 실행할까요? (취소: C / 계속: Enter) : 

if /I "%DO_GIT_SYNC%"=="C" (
    echo.
    call :LOG_WARN "사용자가 git pull을 취소했습니다."
    goto AFTER_GIT_UPDATE
)

echo.
call :LOG_INFO "git pull 실행 중..."
git pull --no-rebase origin %CUR_BRANCH%
if errorlevel 1 (
    echo.
    call :LOG_ERROR "git pull 실행에 실패했습니다."
    call :LOG_ERROR "충돌 또는 기타 Git 문제가 발생했을 수 있습니다."
    goto AFTER_GIT_UPDATE
)

:GIT_UPDATE_SUCCESS
echo.
call :LOG_OK "Git 동기화가 완료되었습니다."

:AFTER_GIT_UPDATE

REM ===============================
REM 3) Subtree 동기화
REM ===============================
call :PRINT_HEADER "Subtree 동기화"

if not exist "%SUBTREE_FILE%" (
    call :LOG_INFO "SubTreeList.txt 파일이 없어 Subtree 동기화를 건너뜁니다."
    goto SUBTREE_SUCCESS
)

call :LOG_INFO "설정 파일 읽는 중: %SUBTREE_FILE%"
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
                call :LOG_ERROR "저장소 정보가 누락되었습니다. Prefix: !LINE_PREFIX!"
                goto FAIL_EXIT
            )

            if "!LINE_BRANCH!"=="" (
                call :LOG_ERROR "브랜치 정보가 누락되었습니다. Prefix: !LINE_PREFIX!"
                goto FAIL_EXIT
            )

            call :SYNC_SUBTREE "!LINE_PREFIX!" "!LINE_REPO!" "!LINE_BRANCH!"
            if errorlevel 1 goto FAIL_EXIT
        )
    )
)

if "!SUBTREE_FOUND!"=="0" (
    call :LOG_INFO "동기화할 유효한 Subtree 항목이 없습니다."
)

:SUBTREE_SUCCESS
echo.
call :LOG_OK "Subtree 동기화가 완료되었습니다."

REM ===============================
REM 4) Submodule 동기화
REM ===============================
call :PRINT_HEADER "Submodule 동기화"

if not exist ".gitmodules" (
    call :LOG_INFO ".gitmodules 파일이 없어 Submodule 동기화를 건너뜁니다."
    goto SUBMODULE_SUCCESS
)

call :LOG_INFO "Submodule 경로 동기화 중..."
git submodule sync --recursive
if errorlevel 1 (
    echo.
    call :LOG_ERROR "git submodule sync 실행에 실패했습니다."
    goto FAIL_EXIT
)

echo.
call :LOG_INFO "Submodule 초기화 및 업데이트 중..."
git submodule update --init --recursive --progress
if errorlevel 1 (
    echo.
    call :LOG_ERROR "submodule init/update 실행에 실패했습니다."
    goto FAIL_EXIT
)

echo.
call :LOG_INFO "Submodule 원격 업데이트 적용 중..."
git submodule update --remote --recursive --progress
if errorlevel 1 (
    echo.
    call :LOG_ERROR "submodule remote update 실행에 실패했습니다."
    goto FAIL_EXIT
)

:SUBMODULE_SUCCESS
echo.
call :LOG_OK "Submodule 동기화가 완료되었습니다."

REM ===============================
REM 5) UE 엔진 경로 불러오기
REM ===============================
call :PRINT_HEADER "UE 엔진 경로 확인"

if exist "%CONFIG_FILE%" (
    set /p UE_ENGINE_DIR=<"%CONFIG_FILE%"
    call :LOG_INFO "저장된 UE 엔진 경로를 불러왔습니다."
    echo        %UE_ENGINE_DIR%
    echo.
) else (
    set "UE_ENGINE_DIR="
)

:VALIDATE_OR_ASK_ENGINE_DIR
if defined UE_ENGINE_DIR (
    call :IS_VALID_ENGINE_DIR "%UE_ENGINE_DIR%"
    if not errorlevel 1 (
        goto ENGINE_DIR_OK
    )

    echo.
    call :LOG_ERROR "저장된 UE 엔진 경로가 올바르지 않습니다."
    call :LOG_ERROR "UnrealBuildTool.exe 또는 Build.bat 파일을 찾지 못했습니다."
    echo        현재 경로: %UE_ENGINE_DIR%
    echo.
)

call :PRINT_HEADER "UE5 엔진 경로 설정"
call :LOG_INFO "예시 경로"
echo        C:\Program Files\Epic Games\UE_5.4
echo        D:\Unreal\UE_5.3
echo.

:ASK_ENGINE_DIR
set "UE_ENGINE_DIR="
set /p UE_ENGINE_DIR=UE 엔진 폴더 경로를 입력하세요: 

if "%UE_ENGINE_DIR%"=="" (
    call :LOG_WARN "엔진 경로가 비어 있습니다. 다시 입력해주세요."
    echo.
    goto ASK_ENGINE_DIR
)

call :IS_VALID_ENGINE_DIR "%UE_ENGINE_DIR%"
if errorlevel 1 (
    echo.
    call :LOG_ERROR "입력한 경로가 올바른 UE 엔진 루트가 아닙니다."
    call :LOG_ERROR "다음 파일들이 존재해야 합니다."
    echo        "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
    echo        "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat"
    echo.
    goto ASK_ENGINE_DIR
)

> "%CONFIG_FILE%" <nul set /p ="%UE_ENGINE_DIR%"
echo.
call :LOG_OK "UE 엔진 경로를 저장했습니다."
echo        %UE_ENGINE_DIR%
echo.

:ENGINE_DIR_OK
set "GENERATE_SCRIPT=%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
set "BUILD_SCRIPT=%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat"

call :LOG_INFO "UnrealBuildTool 경로 확인 완료"
echo        "%GENERATE_SCRIPT%"
call :LOG_INFO "Build.bat 경로 확인 완료"
echo        "%BUILD_SCRIPT%"
echo.

REM ===============================
REM 6) .uproject 자동 탐색
REM ===============================
call :PRINT_HEADER "프로젝트 파일 탐색"

for %%F in ("%SCRIPT_DIR%*.uproject") do (
    if not defined PROJECT_FILE (
        set "PROJECT_FILE=%%~fF"
        set "PROJECT_NAME=%%~nF"
    ) else (
        echo.
        call :LOG_ERROR "스크립트 폴더에 .uproject 파일이 2개 이상 존재합니다."
        call :LOG_ERROR "이 배치는 .uproject 1개만 있는 구조를 기준으로 동작합니다."
        echo.
        echo        발견된 파일:
        echo        - !PROJECT_FILE!
        echo        - %%~fF
        goto FAIL_EXIT
    )
)

if not defined PROJECT_FILE (
    echo.
    call :LOG_ERROR "현재 폴더에서 .uproject 파일을 찾지 못했습니다."
    echo        폴더: %SCRIPT_DIR%
    goto FAIL_EXIT
)

call :LOG_INFO "프로젝트 이름: %PROJECT_NAME%"
call :LOG_INFO "프로젝트 파일:"
echo        "%PROJECT_FILE%"
echo.

REM ===============================
REM 7) 최종 검증
REM ===============================
if not exist "%PROJECT_FILE%" (
    call :LOG_ERROR "프로젝트 파일을 찾을 수 없습니다."
    echo        경로: %PROJECT_FILE%
    goto FAIL_EXIT
)

if not exist "%GENERATE_SCRIPT%" (
    call :LOG_ERROR "UnrealBuildTool.exe 파일을 찾을 수 없습니다."
    echo        경로: %GENERATE_SCRIPT%
    goto FAIL_EXIT
)

if not exist "%BUILD_SCRIPT%" (
    call :LOG_ERROR "Build.bat 파일을 찾을 수 없습니다."
    echo        경로: %BUILD_SCRIPT%
    goto FAIL_EXIT
)

REM ===============================
REM 8) 프로젝트 파일 생성
REM ===============================
call :PRINT_HEADER "Visual Studio 프로젝트 파일 생성"

call :LOG_INFO "GenerateProjectFiles 실행 중..."
call "%GENERATE_SCRIPT%" -projectfiles -project="%PROJECT_FILE%" -game -progress
if errorlevel 1 (
    echo.
    call :LOG_ERROR "Visual Studio 프로젝트 파일 생성에 실패했습니다."
    goto FAIL_EXIT
)

echo.
call :LOG_OK "Visual Studio 프로젝트 파일 생성이 완료되었습니다."

REM ===============================
REM 9) 에디터 빌드
REM ===============================
call :PRINT_HEADER "에디터 빌드"

call :LOG_INFO "%PROJECT_NAME%Editor Win64 Development 빌드 중..."
call "%BUILD_SCRIPT%" %PROJECT_NAME%Editor Win64 Development -project="%PROJECT_FILE%" -WaitMutex -NoHotReload -progress
if errorlevel 1 (
    echo.
    call :LOG_ERROR "빌드에 실패했습니다."
    goto FAIL_EXIT
)

echo.
call :LOG_OK "빌드가 완료되었습니다."

REM ===============================
REM 10) 프로젝트 실행
REM ===============================
call :PRINT_HEADER "프로젝트 실행"

call :LOG_INFO "프로젝트를 실행합니다..."
start "" "%PROJECT_FILE%"
if errorlevel 1 (
    call :LOG_ERROR "프로젝트 실행에 실패했습니다."
    goto FAIL_EXIT
)

echo.
call :LOG_OK "프로젝트 실행 요청이 완료되었습니다."

REM ===============================
REM 11) 정상 종료
REM ===============================
echo.
call :PRINT_HEADER "전체 작업 성공"
timeout /t 3 /nobreak >nul
goto SUCCESS_EXIT

REM ===============================
REM 종료 처리
REM ===============================

:SUCCESS_EXIT
endlocal
exit /b 0

:FAIL_EXIT
echo.
call :PRINT_HEADER "작업 실패"
call :LOG_INFO "위 로그를 확인한 뒤 Git 상태, Subtree/Submodule 상태, 엔진 경로, 프로젝트 파일 상태를 점검해주세요."
pause
endlocal
exit /b 1

REM ===============================
REM 함수: Subtree 동기화
REM ===============================
:SYNC_SUBTREE
set "CURRENT_PREFIX=%~1"
set "CURRENT_REPO=%~2"
set "CURRENT_BRANCH=%~3"

echo --------------------------------------------------
call :LOG_INFO "Subtree 확인 중"
echo         경로   : %CURRENT_PREFIX%
echo         저장소 : %CURRENT_REPO%
echo         브랜치 : %CURRENT_BRANCH%
echo --------------------------------------------------

set "REMOTE_SHA="
for /f "tokens=1" %%R in ('git ls-remote --heads "%CURRENT_REPO%" "%CURRENT_BRANCH%" 2^>nul') do (
    if not defined REMOTE_SHA set "REMOTE_SHA=%%R"
)

if not defined REMOTE_SHA (
    call :LOG_ERROR "원격 HEAD를 확인할 수 없습니다."
    echo         저장소 : %CURRENT_REPO%
    echo         브랜치 : %CURRENT_BRANCH%
    exit /b 1
)

set "LAST_SPLIT="
for /f "tokens=2" %%S in ('git log --grep="git-subtree-dir: %CURRENT_PREFIX%" -n 50 --format^=%%B ^| findstr /b /c:"git-subtree-split:"') do (
    if not defined LAST_SPLIT set "LAST_SPLIT=%%S"
)

if not defined LAST_SPLIT (
    call :LOG_WARN "이전 git-subtree-split 기록을 찾지 못했습니다."
    call :LOG_INFO "동기화 상태를 비교할 수 없어 subtree pull을 실행합니다."

    git subtree pull --prefix="%CURRENT_PREFIX%" "%CURRENT_REPO%" "%CURRENT_BRANCH%" --squash -m "Merge From %CURRENT_PREFIX%"
    if errorlevel 1 (
        call :LOG_ERROR "subtree pull 실행에 실패했습니다. 경로: %CURRENT_PREFIX%"
        exit /b 1
    )

    call :LOG_OK "완료: %CURRENT_PREFIX%"
    echo.
    exit /b 0
)

call :LOG_INFO "마지막 Split : %LAST_SPLIT%"
call :LOG_INFO "원격 HEAD    : %REMOTE_SHA%"

if /I "%LAST_SPLIT%"=="%REMOTE_SHA%" (
    call :LOG_INFO "원격 변경 사항이 없어 subtree pull을 생략합니다."
    echo.
    exit /b 0
)

call :LOG_INFO "원격 변경 사항이 감지되어 subtree pull을 실행합니다..."

git subtree pull --prefix="%CURRENT_PREFIX%" "%CURRENT_REPO%" "%CURRENT_BRANCH%" --squash -m "Merge From %CURRENT_PREFIX%"
if errorlevel 1 (
    call :LOG_ERROR "subtree pull 실행에 실패했습니다. 경로: %CURRENT_PREFIX%"
    exit /b 1
)

call :LOG_OK "완료: %CURRENT_PREFIX%"
echo.
exit /b 0

REM ===============================
REM 함수: UE 엔진 경로 검증
REM ===============================
:IS_VALID_ENGINE_DIR
set "CHECK_ENGINE_DIR=%~1"

if "%CHECK_ENGINE_DIR%"=="" exit /b 1
if not exist "%CHECK_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" exit /b 1
if not exist "%CHECK_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" exit /b 1

exit /b 0

REM ===============================
REM 함수: 섹션 헤더 출력
REM ===============================
:PRINT_HEADER
echo.
echo ==================================================
echo  %~1
echo ==================================================
echo.
exit /b 0

REM ===============================
REM 함수: 로그 출력
REM ===============================
:LOG_INFO
echo [INFO] %~1
exit /b 0

:LOG_WARN
echo [WARN] %~1
exit /b 0

:LOG_ERROR
echo [ERROR] %~1
exit /b 0

:LOG_OK
echo [OK] %~1
exit /b 0