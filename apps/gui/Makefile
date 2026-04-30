WORKSPACE_ROOT := ../..
include $(WORKSPACE_ROOT)/common.mk

# Path to the toolchain's specialized libraries
TOOLCHAIN_LIB_DIR := .toolchain/llvm/lib/x86_64-unknown-linux-gnu

# Specialized LDFLAGS for GUI:
# We use RPATH with $ORIGIN so that the loader can find the toolchain's shared libraries 
# (like libunwind.so.1) relative to the binary's location in bin/.
# This makes the build self-contained without needing global LD_LIBRARY_PATH.
LDFLAGS_GUI := -fuse-ld=lld -stdlib=libc++ -Wl,-rpath,'$$ORIGIN/../$(TOOLCHAIN_LIB_DIR)'

# Raylib paths from vcpkg
RAYLIB_PATH    := $(WORKSPACE_ROOT)/vcpkg/installed/x64-linux
RAYLIB_INCLUDE := $(RAYLIB_PATH)/include
RAYLIB_LIB     := $(RAYLIB_PATH)/lib

# GUI-specific flags
GUI_FLAGS := -I$(RAYLIB_INCLUDE) -I$(WORKSPACE_ROOT)/raylib-src/examples/core

# Final Library List
GUI_LIBS  := -L$(RAYLIB_LIB) -lraylib -lglfw3 \
             -lGL -lm -lpthread -ldl -lrt -lX11 -lXrandr -lXi -lXcursor -lXinerama

TARGET := $(BIN_DIR)/Donjourno-gui

all: $(TARGET)

$(TARGET): main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GUI_FLAGS) $< $(LDFLAGS_GUI) $(GUI_LIBS) -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
