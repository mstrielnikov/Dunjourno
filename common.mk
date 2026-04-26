# Shared Compiler and Flags
# Use $(WORKSPACE_ROOT) to anchor paths to the project root
CXX      := $(WORKSPACE_ROOT)/.toolchain/llvm/bin/clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -g -I$(WORKSPACE_ROOT)/include -I$(WORKSPACE_ROOT)/src/generation -stdlib=libc++
LDFLAGS  := -fuse-ld=lld -static -stdlib=libc++ -lc++abi -lunwind -lpthread -ldl

# Directories
BIN_DIR := $(WORKSPACE_ROOT)/bin
BUILD_DIR := $(WORKSPACE_ROOT)/build
