LINUX_FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt
WINDOWS_FLAGS = -I./raylib_win64/include -L./raylib_win64/lib \
	-lraylib -lopengl32 -lgdi32 -lwinmm \
	-static -static-libgcc -static-libstdc++

.PHONY: build run clean

build:
	mkdir -p build
	g++ src/main.cpp \
		-o build/deck_dungeons \
		$(LINUX_FLAGS)

windows:
	mkdir -p build
	x86_64-w64-mingw32-g++ src/main.cpp \
		-o build/deck_dungeons_windows.exe \
		$(WINDOWS_FLAGS)

run:
	./build/deck_dungeons

clean:
	rm -rf build
