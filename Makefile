test: asm
	./asm test.s

all: asm asm.exe

asm: asm.cpp
	g++ -std=c++20 asm.cpp -o asm

asm.exe: asm.cpp
	x86_64-w64-mingw32-g++ -static -std=c++20 asm.cpp -o asm.exe
