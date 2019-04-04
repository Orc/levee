@echo off

rem set up a tinned windows configuration

if NOT [%1]==[] (                                                           
    set compiler=%1
) ELSE (
    echo usage: configure {compiler}
    exit 1
)

chdir win32

echo copy win32/makefile.%compiler% to Makefile
copy makefile.%compiler% ..\makefile
echo copy win32/config.h to config.h
copy config.h ..\config.h

echo ready to run a make

chdir ..
