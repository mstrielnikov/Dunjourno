WORKSPACE_ROOT := ../..
include $(WORKSPACE_ROOT)/common.mk

TARGET := $(BIN_DIR)/Donjourno-cli

all: $(TARGET)

$(TARGET): main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
