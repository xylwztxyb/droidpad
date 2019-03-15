@ECHO OFF
ECHO ========================
ECHO Input Method Installing...
ECHO ========================
cd /d %~dp0
%1 start "" mshta vbscript:createobject("shell.application").shellexecute("""%~0""","::",,"runas",1)(window.close)&exit

SET INSTALL_DIR="%ProgramFiles(x86)%\DroidPad"

ECHO *************** CHECK THE INSTALL_DIR...
IF EXIST %INSTALL_DIR% (
	ECHO Dir %INSTALL_DIR% exist, skip create it.
) ELSE (
	ECHO Make dir %INSTALL_DIR%
	MD %INSTALL_DIR%
)

ECHO *************** INSTALL FILES...

ECHO COPY:DroidPadServer.exe _ %INSTALL_DIR%\
COPY /Y DroidPadServer.exe %INSTALL_DIR%\DroidPadServer.exe

ECHO COPY:adb.exe _ %INSTALL_DIR%\
COPY /Y adb.exe %INSTALL_DIR%\adb.exe

ECHO COPY:AdbWinApi.dll _ %INSTALL_DIR%\
COPY /Y AdbWinApi.dll %INSTALL_DIR%\AdbWinApi.dll

ECHO COPY:installer.exe _ %INSTALL_DIR%\
COPY /Y installer.exe %INSTALL_DIR%\installer.exe

ECHO COPY:res _ %INSTALL_DIR%\
XCOPY res %INSTALL_DIR%\res\ /s /y

ECHO COPY DroidPadIME_win32.ime _ %SystemRoot%\SysWOW64\DroidPadIME.ime
COPY /Y DroidPadIME_win32.ime %SystemRoot%\SysWOW64\DroidPadIME.ime

ECHO COPY DroidPadIME_x64.ime _ %SystemRoot%\System32\DroidPadIME.ime
COPY /Y DroidPadIME_x64.ime %SystemRoot%\System32\DroidPadIME.ime

ECHO ***************  SETTING REGISTRY...
REG ADD HKCU\Software\DroidPadServer /f
REG ADD HKCU\Software\DroidPadServer /v ROOT /t REG_SZ /d ^%INSTALL_DIR% /f
REG ADD HKCU\Software\DroidPadServer /v BIN /t REG_SZ /d DroidPadServer.exe /f

ECHO ***************  INSTALL IME...
PAUSE
%INSTALL_DIR%\installer.exe /i %SystemRoot%\System32\DroidPadIME.ime

ECHO ***************  Done

PAUSE