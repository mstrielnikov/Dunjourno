# Compiler and Linker
CXX      := .toolchain/llvm/bin/clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -g -Iinclude -Isrc/generation -stdlib=libc++
LDFLAGS  := -fuse-ld=lld -static -stdlib=libc++ -lc++abi -lunwind -lpthread -ldl

# Project Structure
SRC_DIR  := src
BUILD_DIR := build
TARGET   := AutomataDungeone

# Sources and Objects
# Find all .cpp files in current dir and src/
SRCS     := $(wildcard *.cpp) $(wildcard $(SRC_DIR)/*.cpp)
# Objects should be in build/ and mirror the source path
OBJS     := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Default Rule
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

# Compiling source files to object files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Cleanup
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
