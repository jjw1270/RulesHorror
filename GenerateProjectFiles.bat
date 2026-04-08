@echo off
setlocal EnableExtensions
chcp 65001 >nul

REM ===============================
REM 설정
REM ===============================
set "SCRIPT_DIR=%~dp0"
set "CONFIG_FILE=%SCRIPT_DIR%UE_EngineDir.txt"
set "UE_ENGINE_DIR="
set "GENERATE_SCRIPT="
set "PROJECT_FILE="

call :PRINT_HEADER "Unreal Engine 프로젝트 파일 생성"

REM ===============================
REM 1) 저장된 엔진 경로 불러오기
REM ===============================
if exist "%CONFIG_FILE%" (
    set /p UE_ENGINE_DIR=<"%CONFIG_FILE%"
    call :LOG_INFO "저장된 UE 엔진 경로를 불러왔습니다."
    call :LOG_INFO "%UE_ENGINE_DIR%"
    echo.
)

REM ===============================
REM 2) 엔진 경로 검증 / 재입력
REM ===============================
:VALIDATE_OR_ASK_ENGINE_DIR
if not "%UE_ENGINE_DIR%"=="" (
    call :IS_VALID_ENGINE_DIR "%UE_ENGINE_DIR%"
    if not errorlevel 1 (
        goto ENGINE_DIR_OK
    )

    echo.
    call :LOG_ERROR "저장된 UE 엔진 경로가 올바르지 않습니다."
    call :LOG_ERROR "UnrealBuildTool.exe 또는 Build.bat 파일을 찾지 못했습니다."
    call :LOG_ERROR "현재 경로: %UE_ENGINE_DIR%"
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
    call :LOG_ERROR "다음 파일들을 확인할 수 있어야 합니다."
    echo         "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
    echo         "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat"
    echo.
    goto ASK_ENGINE_DIR
)

> "%CONFIG_FILE%" <nul set /p ="%UE_ENGINE_DIR%"
echo.
call :LOG_OK "UE 엔진 경로를 저장했습니다."
echo      %UE_ENGINE_DIR%
echo.

:ENGINE_DIR_OK
set "GENERATE_SCRIPT=%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

call :LOG_INFO "UnrealBuildTool 경로 확인 완료"
echo        "%GENERATE_SCRIPT%"
echo.

REM ===============================
REM 3) .uproject 자동 탐색
REM ===============================
for %%F in ("%SCRIPT_DIR%*.uproject") do (
    set "PROJECT_FILE=%%~fF"
    goto PROJECT_FILE_FOUND
)

:PROJECT_FILE_FOUND
if "%PROJECT_FILE%"=="" (
    echo.
    call :LOG_ERROR "현재 폴더에서 .uproject 파일을 찾지 못했습니다."
    call :LOG_ERROR "폴더: %SCRIPT_DIR%"
    goto FAIL_EXIT
)

call :LOG_INFO "프로젝트 파일 확인 완료"
echo        "%PROJECT_FILE%"
echo.

REM ===============================
REM 4) 프로젝트 파일 생성
REM ===============================
call :PRINT_HEADER "Visual Studio 프로젝트 파일 생성"

call :LOG_INFO "GenerateProjectFiles 실행 중..."
"%GENERATE_SCRIPT%" -projectfiles -project="%PROJECT_FILE%" -game -progress
if errorlevel 1 (
    echo.
    call :LOG_ERROR "Visual Studio 프로젝트 파일 생성에 실패했습니다."
    goto FAIL_EXIT
)

echo.
call :PRINT_HEADER "작업 성공"
call :LOG_OK "Visual Studio 프로젝트 파일 생성이 완료되었습니다."
goto SUCCESS_EXIT

REM ===============================
REM 종료 처리
REM ===============================

:SUCCESS_EXIT
timeout /t 2 /nobreak >nul
endlocal
exit /b 0

:FAIL_EXIT
echo.
call :PRINT_HEADER "작업 실패"
call :LOG_INFO "위 로그를 확인한 뒤 경로 또는 프로젝트 상태를 점검해주세요."
pause
endlocal
exit /b 1

REM ===============================
REM 함수: 엔진 경로 검증
REM ===============================
:IS_VALID_ENGINE_DIR
set "CHECK_ENGINE_DIR=%~1"

if "%CHECK_ENGINE_DIR%"=="" exit /b 1
if not exist "%CHECK_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" exit /b 1
if not exist "%CHECK_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" exit /b 1

exit /b 0

REM ===============================
REM 함수: 헤더 출력
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