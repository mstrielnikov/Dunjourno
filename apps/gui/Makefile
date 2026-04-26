WORKSPACE_ROOT := ../..
include $(WORKSPACE_ROOT)/common.mk

# GUI-specific flags (e.g. for raylib)
# For now, it's identical to CLI for the mock
GUI_FLAGS := 
TARGET := $(BIN_DIR)/dunjourno-gui

all: $(TARGET)

$(TARGET): main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GUI_FLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
