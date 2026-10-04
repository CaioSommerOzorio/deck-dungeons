FLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt

.PHONY: build run clean

build:
	mkdir -p build
	g++ src/main.cpp \
		-o build/deck_dungeons \
		$(FLAGS)

run:
	./build/deck_dungeons

clean:
	rm -rf build
