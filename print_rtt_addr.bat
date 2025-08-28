@echo off
setlocal enabledelayedexpansion

set MAPFILE=%1

REM アドレスを探す
for /f "tokens=1,2 delims= " %%A in ('findstr /r /c:"[0-9A-Fa-fx][0-9A-Fa-fx]* *[_]SEGGER_RTT" %MAPFILE%') do (
    if "%%B"=="_SEGGER_RTT" (
        echo SEGGER_RTT address: %%A
        goto :eof
    )
)

echo SEGGER_RTT address not found.
