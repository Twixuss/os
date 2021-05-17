@echo off

pushd src

nasm -f bin boot.asm -o ..\bin\boot.bin

echo Success
goto end
:fail
echo Fail
:end

popd
