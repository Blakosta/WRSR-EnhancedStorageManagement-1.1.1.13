@echo off
setlocal EnableExtensions
set "ROOT=%~dp0"
if "%~1"=="" (
  set "OUT=%ROOT%out"
) else (
  set "OUT=%~1"
)
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%\runtime\rml\plugins" || exit /b 1
mkdir "%OUT%\base" || exit /b 1

where clang-cl >nul 2>nul || (echo clang-cl not found in PATH & exit /b 1)
where lld-link >nul 2>nul || (echo lld-link not found in PATH & exit /b 1)
where py >nul 2>nul || (echo Python launcher ^(py^) not found in PATH & exit /b 1)

lld-link /lib /machine:x64 /def:"%ROOT%kernel32_min.def" /out:"%OUT%\kernel32_min.lib" || exit /b 1
clang-cl /nologo /c /O2 /GS- /GR- /EHs-c- /Zl /W4 /WX /Fo"%OUT%\EnhancedStorageManagement.obj" "%ROOT%EnhancedStorageManagement.cpp" || exit /b 1
lld-link /nologo /dll /machine:x64 /entry:DllMain /subsystem:windows /nodefaultlib /dynamicbase /highentropyva /nxcompat /timestamp:0 /opt:ref /opt:icf /out:"%OUT%\base\EnhancedStorageManagement.dll" "%OUT%\EnhancedStorageManagement.obj" "%OUT%\kernel32_min.lib" || exit /b 1

py -3 "%ROOT%finalize_pe.py" "%OUT%\base\EnhancedStorageManagement.dll" "%OUT%\runtime\rml\plugins\EnhancedStorageManagement.dll" || exit /b 1
copy /y "%ROOT%EnhancedStorageManagement.ini" "%OUT%\runtime\rml\plugins\" >nul || exit /b 1
copy /y "%ROOT%EnhancedStorageManagement.rml.json" "%OUT%\runtime\rml\plugins\" >nul || exit /b 1
py -3 "%ROOT%verify_manifest.py" "%OUT%\runtime\rml\plugins\EnhancedStorageManagement.rml.json" "%OUT%\runtime\rml\plugins" || exit /b 1
py -3 "%ROOT%verify_pe.py" "%OUT%\runtime\rml\plugins\EnhancedStorageManagement.dll" || exit /b 1
certutil -hashfile "%OUT%\runtime\rml\plugins\EnhancedStorageManagement.dll" SHA256
exit /b %errorlevel%
