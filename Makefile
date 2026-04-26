# Master Makefile - Recursive Build System

all: cli gui

cli:
	$(MAKE) -C apps/cli

gui:
	$(MAKE) -C apps/gui

clean:
	$(MAKE) -C apps/cli clean
	$(MAKE) -C apps/gui clean
	rm -rf bin build

.PHONY: all cli gui clean
