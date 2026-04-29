WORKSPACE_ROOT := ../..

# ── EMSDK Sandbox ────────────────────────────────
# All toolchain deps live inside the project directory.
# No system-wide installs required — run `make setup-wasm` from root to bootstrap.

PYTHON_BIN  := $(abspath $(WORKSPACE_ROOT)/python/bin)
EMSDK_DIR   := $(abspath $(WORKSPACE_ROOT)/emsdk)
EMCC_DIR    := $(EMSDK_DIR)/upstream/emscripten

export PATH            := $(PYTHON_BIN):$(EMCC_DIR):$(EMSDK_DIR):$(PATH)
export EMSDK_PYTHON    := $(PYTHON_BIN)/python3.11
export EMSDK           := $(EMSDK_DIR)

# ── Compiler ─────────────────────────────────────

CXX      := em++
CXXFLAGS := -std=c++23 -Wall -Wextra -Wno-missing-field-initializers -Os \
            -I$(WORKSPACE_ROOT)/include \
            -I$(WORKSPACE_ROOT)/src \
            -I$(WORKSPACE_ROOT)/raylib-src/src/external \
            -DPLATFORM_WEB

# ── Raylib (built from source for PLATFORM_WEB) ──

RAYLIB_SRC     := $(WORKSPACE_ROOT)/raylib-src/src
RAYLIB_INCLUDE := $(RAYLIB_SRC)
RAYLIB_LIB     := $(RAYLIB_SRC)/libraylib.web.a

# ── Emscripten Linker ────────────────────────────

LDFLAGS := -s USE_GLFW=3 \
           -s ASYNCIFY \
           -s TOTAL_MEMORY=512MB \
           --shell-file $(RAYLIB_SRC)/shell.html

# ── Output ───────────────────────────────────────

OUT_DIR := $(WORKSPACE_ROOT)/build-wasm
TARGET  := $(OUT_DIR)/index.html

all: $(TARGET)

$(TARGET): main.cpp $(RAYLIB_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(RAYLIB_INCLUDE) $< $(RAYLIB_LIB) $(LDFLAGS) -o $@
	@echo ""
	@echo "Build complete: $(TARGET)"
	@echo "Run 'make serve' from project root."

$(RAYLIB_LIB):
	@echo "Building Raylib for WebAssembly..."
	@test -d "$(RAYLIB_SRC)" || { echo "Error: raylib-src not found. Run 'make setup-wasm' from project root."; exit 1; }
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_WEB -j$$(nproc)

clean:
	rm -f $(OUT_DIR)/index.html $(OUT_DIR)/index.js $(OUT_DIR)/index.wasm $(OUT_DIR)/index.data

clean-all: clean
	$(MAKE) -C $(RAYLIB_SRC) clean 2>/dev/null || true

.PHONY: all clean clean-all
