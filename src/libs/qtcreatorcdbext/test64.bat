@echo off

REM test.bat: Test script to launch CDB.exe using the extension
REM with the tests/manual demo project.

REM !qtcreatorcdbext.help
REM !qtcreatorcdbext.assign local.this.m_w=44
REM !qtcreatorcdbext.locals 0

set ROOT=c:\qt\4.7-vs8\creator

set _NT_DEBUGGER_EXTENSION_PATH=%ROOT%\lib\qtcreatorcdbext64
set EXT=qtcreatorcdbext.dll
set EXE=%ROOT%\tests\manual\gdbdebugger\gui\debug\gui.exe

set CDB=C:\PROGRA~1\DEBUGG~1\cdb.exe

echo %CDB%

echo "!qtcreatorcdbext.pid"

REM Launch emulating cdbengine's setup with idle reporting
%CDB% -G -a%EXT% -c ".idle_cmd ^!qtcreatorcdbext.idle" %EXE%
