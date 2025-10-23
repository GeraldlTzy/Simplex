OUT_DIR := build

# ==== SOURCE FILES ====
SRC_MAIN     := $(filter-out src/include/%,$(wildcard src/*.c))
SRC_INCLUDE  := $(wildcard src/include/*.c)
SRC_ALL      := $(SRC_MAIN) $(SRC_INCLUDE)

# ==== BINARY FILES ====
BIN_MAIN     := $(OUT_DIR)/main


CFLAGS := $(shell pkg-config --cflags gtk+-3.0) -I src/include -MMD -MP
LIBS   := $(shell pkg-config --libs gtk+-3.0) -rdynamic -lfontconfig -lm

.PHONY: all clean run

# === COMPILE MAIN PROGRAM ===
$(BIN_MAIN): $(SRC_ALL)
	@mkdir -p $(OUT_DIR)
	@gcc $^ -o $@ $(CFLAGS) $(LIBS) && chmod +x $@ \
		&& echo "✅ $@ BUILT SUCCESSFULLY" || echo "❌ $@ HAD AN ERROR"

# === RUN MAIN PROGRAM ===
run: $(BIN_MAIN)
	@$(BIN_MAIN)

# === CLEAR OUT DIRECTORY ===
clean:
	@rm -rf $(OUT_DIR)
