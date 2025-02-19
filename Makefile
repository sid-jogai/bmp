CC = cc
DEBUG_FLAGS = -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g3
RELEASE_FLAGS = -O3
SDL3 = $$(pkg-config --cflags --libs SDL3)

release:
	$(CC) $(RELEASE_FLAGS) main.c $(SDL3) -o bmp

debug:
	$(CC) $(DEBUG_FLAGS) main.c $(SDL3) -o bmp

