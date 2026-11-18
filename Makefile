CC      ?= gcc
CFLAGS  = -std=gnu11 -D_GNU_SOURCE -Wall -Wextra -Wpedantic -Werror -g -O2 -Iinclude
DEPFLAGS = -MMD -MP
SRC     = src/main.c src/server.c src/http.c src/mime.c src/files.c
OBJ     = $(SRC:src/%.c=build/%.o)
DEP     = $(OBJ:.o=.d)
TARGET  = minihttpd

.PHONY: all debug clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# -MMD -MP genera build/*.d con los .h de cada .c, asi que si cambias un .h
# se vuelven a compilar los .c que lo incluyen
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

build:
	mkdir -p build

# Compilación de depuración con AddressSanitizer y UBSan
debug: CFLAGS = -std=gnu11 -D_GNU_SOURCE -Wall -Wextra -Wpedantic -Werror -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude
debug:
	$(CC) $(CFLAGS) -o $(TARGET)_debug $(SRC)

clean:
	rm -rf build $(TARGET) $(TARGET)_debug

-include $(DEP)
