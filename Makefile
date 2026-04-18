CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
SRC = src/main.c
BIN = build/zeros
ISO_DIR = build/iso_root
ISO_NAME = build/ZerOS-1.0.iso

.PHONY: all run clean iso

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

iso: $(BIN)
	@mkdir -p $(ISO_DIR)
	cp $(BIN) $(ISO_DIR)/zeros
	printf "ZerOS 1.0 installation media (simulation).\n" > $(ISO_DIR)/README.txt
	@if command -v xorriso >/dev/null 2>&1; then \
		xorriso -as mkisofs -o $(ISO_NAME) $(ISO_DIR) >/dev/null 2>&1 && echo "ISO created: $(ISO_NAME)"; \
	elif command -v genisoimage >/dev/null 2>&1; then \
		genisoimage -o $(ISO_NAME) $(ISO_DIR) >/dev/null 2>&1 && echo "ISO created: $(ISO_NAME)"; \
	else \
		echo "xorriso/genisoimage not found. Install one of them to build ISO."; \
		exit 1; \
	fi

clean:
	rm -rf build
