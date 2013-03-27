rem # find & move absolute path
set ABS_PATH=%~dp0
cd %ABS_PATH%

set PWD=%CD%
set ACTIVATOR_PATH=%PWD%
set ACTIVATOR_BIN=WindowActivator.exe
set PATH=%ACTIVATOR_PATH%;%PATH%

%ACTIVATOR_PATH%\%ACTIVATOR_BIN% %*
