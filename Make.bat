call cmake_gcc
call cmake_gcc x64
copy /Y build\test.exe bin\winIdent32.exe
copy /Y build64\test.exe bin\winIdent64.exe
