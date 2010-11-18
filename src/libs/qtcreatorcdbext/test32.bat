@echo off

REM test.bat: Test script to launch CDB.exe using the extension
REM with the tests/manual demo project.

REM !qtcreatorcdbext.help
REM !qtcreatorcdbext.assign local.this.m_w=44
REM !qtcreatorcdbext.locals 0

set ROOT=d:\dev\qt4.7-vs8\creator

set _NT_DEBUGGER_EXTENSION_PATH=%ROOT%\lib\qtcreatorcdbext32
set EXT=qtcreatorcdbext.dll
set EXE=%ROOT%\tests\manual\gdbdebugger\gui\debug\gui.exe

set CDB=D:\Programme\Debugging Tools for Windows (x86)\cdb.exe

echo %CDB%
echo %EXT% %_NT_DEBUGGER_EXTENSION_PATH%

echo "!qtcreatorcdbext.pid"

REM Launch emulating cdbengine's setup with idle reporting
"%CDB%" -G -a%EXT% -c ".idle_cmd !qtcreatorcdbext.idle" %EXE%
