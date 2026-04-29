# Shared Compiler and Flags
# Use $(WORKSPACE_ROOT) to anchor paths to the project root
CXX      := $(WORKSPACE_ROOT)/.toolchain/llvm/bin/clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -Wno-missing-field-initializers -g -I$(WORKSPACE_ROOT)/include -I$(WORKSPACE_ROOT)/src -stdlib=libc++
LDFLAGS  := -fuse-ld=lld -static -stdlib=libc++ -lc++abi -lunwind -lpthread -ldl

# Directories
BIN_DIR := $(WORKSPACE_ROOT)/bin
BUILD_DIR := $(WORKSPACE_ROOT)/build
