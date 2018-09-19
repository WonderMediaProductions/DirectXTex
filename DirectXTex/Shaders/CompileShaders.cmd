@echo off
rem Copyright (c) Microsoft Corporation. All rights reserved.
rem Licensed under the MIT License.

setlocal
set error=0

set FXCOPTS=/nologo /WX /Ges /Zi /Zpc /Qstrip_reflect /Qstrip_debug

set PCFXC="%WindowsSdkBinPath%%WindowsSDKVersion%\x86\fxc.exe"
if exist %PCFXC% goto continue
set PCFXC="%WindowsSdkDir%bin\%WindowsSDKVersion%\x86\fxc.exe"
if exist %PCFXC% goto continue
set PCFXC="%WindowsSdkDir%bin\x86\fxc.exe"
if exist %PCFXC% goto continue

set PCFXC=fxc.exe

:continue
@if not exist Compiled mkdir Compiled

call :CompileShader vs_4_0 IBLSampler VS_CubeMap
call :CompileShader gs_4_0 IBLSampler GS_CubeMap
call :CompileShader ps_4_0 IBLSampler PS_SpecularCubeMap
call :CompileShader ps_4_0 IBLSampler PS_DiffuseCubeMap

call :CompileShader cs_4_0 BC7Encode TryMode456CS
call :CompileShader cs_4_0 BC7Encode TryMode137CS
call :CompileShader cs_4_0 BC7Encode TryMode02CS
call :CompileShader cs_4_0 BC7Encode EncodeBlockCS

call :CompileShader cs_4_0 BC6HEncode TryModeG10CS
call :CompileShader cs_4_0 BC6HEncode TryModeLE10CS
call :CompileShader cs_4_0 BC6HEncode EncodeBlockCS

echo.

if %error% == 0 (
    echo Shaders compiled ok
) else (
    echo There were shader compilation errors!
)

endlocal
exit /b

:CompileShader
set fxc=%PCFXC% %2.hlsl %FXCOPTS% /T%1 /E%3 /FhCompiled\%2_%3.inc /FdCompiled\%2_%3.pdb /Vn%2_%3
echo.
echo %fxc%
%fxc% || set error=1
exit /b
