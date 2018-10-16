CC = clang
BUILD_DIR = _build

CFLAGS = -O3 -Wall -Wextra -pedantic -Wno-unused-parameter -std=c11 -g -I$(BUILD_DIR) -fsanitize=address
LDFLAGS =


absc: src/main.c $(BUILD_DIR)/scheme.o $(BUILD_DIR)/sexp.o $(BUILD_DIR)/term.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.h src/%.c | $(BUILD_DIR)
	$(CC) -c -o $@ src/$*.c $(CFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*
	rm absc
