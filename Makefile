# Master Makefile for Donjourno Recursive Build System
# ──────────────────────────────────────────────────
# Targets:
#   make cli        Build native CLI (LLVM C++23)
#   make gui        Build native GUI (LLVM C++23 + Raylib)
#   make wasm       Build WebAssembly (Emscripten + Raylib)
#   make run        Run native GUI
#   make serve      Build & serve WebAssembly via Bun
#   make clean      Remove build artifacts
#   make setup-wasm Bootstrap EMSDK + Python sandbox

all: gui

gui:
	$(MAKE) -C apps/gui

wasm:
	$(MAKE) -C apps/wasm

# ── Run ──────────────────────────────────────────

run: gui
	@./bin/Donjourno-gui

serve: wasm
	@echo "Serving at http://localhost:8080"
	@bun run tools/serve.ts

# ── Wasm Toolchain Bootstrap ─────────────────────

setup-wasm:
	@echo "=== Setting up EMSDK sandbox ==="
	@test -d emsdk || git clone https://github.com/emscripten-core/emsdk.git emsdk
	@cd emsdk && ./emsdk install latest && ./emsdk activate latest
	@echo "=== Fetching standalone Python 3.11 ==="
	@test -d python || ( \
		curl -sL https://github.com/indygreg/python-build-standalone/releases/download/20240415/cpython-3.11.9+20240415-x86_64-unknown-linux-gnu-install_only.tar.gz | tar xz && \
		ln -sf python3.11 python/bin/python3 \
	)
	@echo "=== Fetching Raylib source ==="
	@test -d raylib-src || git clone --depth 1 https://github.com/raysan5/raylib.git raylib-src
	@echo "Done. Run 'make wasm' to build."

# ── Clean ────────────────────────────────────────

clean:
	$(MAKE) -C apps/gui clean
	$(MAKE) -C apps/wasm clean 2>/dev/null || true
	rm -rf bin build

clean-all: clean
	$(MAKE) -C apps/wasm clean-all 2>/dev/null || true

.PHONY: all gui wasm run serve setup-wasm clean clean-all
