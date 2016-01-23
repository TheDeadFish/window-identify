call egcc.bat
cd obj
windres ..\test.rc -O res -o test.res
windres ..\..\src\winidentify.rc -o winidentify.res
rescat res.out *.res
windres -J res res.out -o res.o
gcc ..\*.cpp ..\..\src\*.cpp -c -Os -fomit-frame-pointer
gcc *.o -s -lgdi32 -lpsapi -o ..\test.exe -mwindows
cd..
