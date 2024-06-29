@echo off
echo Compiling Fractint with Borland C
echo .
make -fbcfract.mak fractint.exe > bcfract.log
echo Done! Check bcfract.log below:
type bcfract.log | more
