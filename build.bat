@echo off
setlocal enabledelayedexpansion

cd /D "%~dp0"

rem Record start time
set "start=%TIME%"

for %%a in (%*) do set "%%a=1"

if not "%release%"=="1" set debug=1

if "%debug%"=="1"   set release=0 && echo [Debug Mode]
if "%release%"=="1" set debug=0 && echo [Release Mode]
if "%msvc%"=="1"    set clang=0 && echo [Compiler: MSVC]
if "%clang%"=="1"   set msvc=0 && echo [Compiler: Clang]

set auto_compile_flags=
if "%telemetry%"=="1" set auto_compile_flags=%auto_compile_flags% -DBUILD_TELEMETRY=1 && echo [telemetry profiling enabled]
if "%asan%"=="1"      set auto_compile_flags=%auto_compile_flags% -fsanitize=address && echo [ASAN enabled]

set cl_common=     /I..\src\ /I..\third_party\ /I..\local\ /std:c++20 /nologo /FC /Z7 /EHsc
set clang_common=  -I..\src\ -I..\third_party\ -I..\local\ -std=c++20 -gcodeview -fdiagnostics-absolute-paths -Wall -Wno-unknown-warning-option -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Xclang -flto-visibility-public-std -maes -msse4 -mssse3 -Wno-macro-redefined -Wno-initializer-overrides -Wno-visibility

rem Check for Vulkan SDK
if defined VULKAN_SDK (
	echo [Using Vulkan SDK from %VULKAN_SDK%]
	set "VULKAN_SDK=%VULKAN_SDK:"=%"
	set "cl_common=%cl_common% /I%VULKAN_SDK%\Include"
	set "clang_common=%clang_common% -I%VULKAN_SDK%/Include"
) else (
	echo [WARNING] VULKAN_SDK environment variable not set.
)

rem Compile GLSL shaders into SPIR-V
if "%shaders%"=="1" (
	set "GLSLC=!VULKAN_SDK!\Bin\glslc.exe"

	if exist "!GLSLC!" (
		echo [Compiling GLSL shaders to SPIR-V...]
		for %%F in (res\shaders\*.vert res\shaders\*.frag res\shaders\*.comp) do (
			if exist "%%F" (
				set "OUTFILE=res\shaders\%%~nF%%~xF.spv"
				echo   %%~xF ^>^> !OUTFILE!
				"!GLSLC!" "%%F" -o "!OUTFILE!" || (
					echo [ERROR] Shader compile failed: %%F
					exit /b 1
				)
			)
		)
	) else (
		echo [WARNING] glslc not found at "!GLSLC!". Skipping shader compilation.
	)
)

rem Don't move forward if no compiler selected
if not "%msvc%"=="1" if not "%clang%"=="1" (
	exit /b 0
)

set cl_debug=      call cl /Od /DDEBUG=1 %cl_common% %auto_compile_flags%
set cl_release=    call cl /O2 /DDEBUG=0 %cl_common% %auto_compile_flags%
set clang_debug=   call clang -g -O0 -DDEBUG=1 %clang_common% %auto_compile_flags%
set clang_release= call clang -g -O2 -DDEBUG=0 %clang_common% %auto_compile_flags%
set cl_link=       /link /MANIFEST:EMBED /INCREMENTAL:NO /pdbaltpath:%%%%_PDB%%%%
set clang_link=    -fuse-ld=lld -Xlinker /MANIFEST:EMBED -Xlinker /pdbaltpath:%%%%_PDB%%%%
set cl_out=        /out:
set clang_out=     -o

set link_dll=-DLL
if "%msvc%"=="1"  set only_compile=/c
if "%clang%"=="1" set only_compile=-c
if "%msvc%"=="1"  set no_aslr=/DYNAMICBASE:NO
if "%clang%"=="1" set no_aslr=-Wl,/DYNAMICBASE:NO
if "%msvc%"=="1"  set rc=call rc
if "%clang%"=="1" set rc=call llvm-rc

if "%msvc%"=="1"      set compile_debug=%cl_debug%
if "%msvc%"=="1"      set compile_release=%cl_release%
if "%msvc%"=="1"      set compile_link=%cl_link%
if "%msvc%"=="1"      set out=%cl_out%
if "%clang%"=="1"     set compile_debug=%clang_debug%
if "%clang%"=="1"     set compile_release=%clang_release%
if "%clang%"=="1"     set compile_link=%clang_link%
if "%clang%"=="1"     set out=%clang_out%
if "%debug%"=="1"     set compile=%compile_debug%
if "%release%"=="1"   set compile=%compile_release%

if not exist build mkdir build
if not exist local mkdir local

for /f %%i in ('call git describe --always --dirty')   do set compile=%compile% -DBUILD_GIT_HASH=\"%%i\"
for /f %%i in ('call git rev-parse HEAD')              do set compile=%compile% -DBUILD_GIT_HASH_FULL=\"%%i\"

if "%msvc%"=="1"  set compile=%compile% -DCOMPILER_MSVC="1"
if "%clang%"=="1" set compile=%compile% -DCOMPILER_CLANG="1"
set compile=%compile% -DRENDER_BACKEND_VULKAN="1"

if "%profiling%"=="1" set compile=%compile% -DENABLE_PROFILING="1"

pushd build
set didbuild=1 && %compile% ..\src\main.cpp %compile_link% %out%main.exe || exit /b 1
popd

rem Record end time
set "end=%TIME%"

if "%didbuild%"=="" (
	echo [WARNING] No valid build target specified.
	exit /b 1
)

:: cleanup
for %%a in (%*) do set "%%a=0"
set compile=
set compile_link=
set out=
set msvc=
set debug=

rem Convert start and end time to milliseconds
call :timeToMilliseconds "%start%" startMS
call :timeToMilliseconds "%end%" endMS

rem Handle case when build spans midnight
if !endMS! LSS !startMS! (
	set /a durationMS=!endMS! + 86400000 - !startMS!
) else (
	set /a durationMS=!endMS! - !startMS!
)

rem Split into seconds and milliseconds
set /a durationSec=!durationMS! / 1000
set /a durationMilli=!durationMS! %% 1000

echo Build completed in !durationSec!.!durationMilli! seconds.
exit /b 0

:timeToMilliseconds
rem %1 = time string, %2 = output variable
setlocal
set "timestr=%~1"
for /f "tokens=1-4 delims=:.," %%a in ("%timestr%") do (
	set /a "hh=1%%a - 100"
	set /a "mm=1%%b - 100"
	set /a "ss=1%%c - 100"
	set /a "cs=1%%d - 100"
)
rem cs = centiseconds; convert to milliseconds (multiply by 10)
set /a totalMS=(hh * 3600 + mm * 60 + ss) * 1000 + (cs * 10)
endlocal & set "%~2=%totalMS%"
