@echo off
setlocal

REM ===============================
REM 설정 파일: UE 엔진 경로 저장용
REM ===============================
set CONFIG_FILE=%~dp0UE_EngineDir.txt

REM ===============================
REM 0) UE_ENGINE_DIR 로드 또는 최초 입력
REM ===============================
if exist "%CONFIG_FILE%" (
    set /p UE_ENGINE_DIR=<"%CONFIG_FILE%"
) else (
    set UE_ENGINE_DIR=
)

:VALIDATE_ENGINE_DIR
if not "%UE_ENGINE_DIR%"=="" (
    if exist "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" (
        if exist "%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" (
            goto ENGINE_DIR_OK
        )
    )
    echo.
    echo [ERROR] 저장된 UE 엔진 경로가 올바르지 않습니다.
    echo        UnrealBuildTool.exe 또는 Build.bat 을 찾지 못했습니다.
    echo        현재 경로: %UE_ENGINE_DIR%
    echo.
)

echo ============================================
echo  UE5 엔진 경로 설정
echo  예) C:\Program Files\Epic Games\UE_5.4
echo      D:\Unreal\UE_5.3
echo.

:ASK_ENGINE_DIR
set UE_ENGINE_DIR=
set /p UE_ENGINE_DIR=UE 엔진 폴더 경로를 입력하세요: 

if "%UE_ENGINE_DIR%"=="" (
    echo 엔진 경로가 비어 있습니다. 다시 입력하세요.
    echo.
    goto ASK_ENGINE_DIR
)

if not exist "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" (
    echo.
    echo [ERROR] UnrealBuildTool.exe 를 찾을 수 없습니다.
    echo 경로: "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
    echo UE 엔진 루트 경로가 맞는지 다시 확인해주세요.
    echo.
    goto ASK_ENGINE_DIR
)

> "%CONFIG_FILE%" echo %UE_ENGINE_DIR%
echo.
echo [INFO] UE 엔진 경로를 저장했습니다: %UE_ENGINE_DIR%

:ENGINE_DIR_OK

set "GENERATE_SCRIPT=%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

REM ===============================
REM PROJECT_FILE 자동 탐색
REM ===============================
set "PROJECT_FILE="

for %%F in ("%~dp0*.uproject") do (
    set "PROJECT_FILE=%%~fF"
    goto PROJECT_FILE_FOUND
)

:PROJECT_FILE_FOUND
if "%PROJECT_FILE%"=="" (
    echo.
    echo [ERROR] 현재 폴더에서 .uproject 를 찾지 못했습니다.
    echo        폴더: %~dp0
    echo.
    pause
    exit /b 1
)

echo [INFO] PROJECT_FILE: "%PROJECT_FILE%"
echo.

echo Generate Visual Studio project files...
call "%GENERATE_SCRIPT%" -projectfiles -project="%PROJECT_FILE%" -game -progress
if errorlevel 1 (
    echo.
    echo ***** GenerateProjectFiles FAILED *****
    pause
    exit /b 1
)

endlocal
exit /b 0
