WORKSPACE_ROOT := ../..
include $(WORKSPACE_ROOT)/common.mk

TARGET := $(BIN_DIR)/Donjourno-cli

all: $(TARGET)

$(TARGET): main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(WORKSPACE_ROOT)/vcpkg/installed/x64-linux/include $< $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
