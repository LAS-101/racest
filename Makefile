# CMake owns compilation and dependency tracking. Make provides task shortcuts.
CMAKE ?= cmake
PYTHON ?= python3
JOBS ?= 2
NATIVE_BUILD := build/native
WEB_BUILD := build/web

.PHONY: all build run agent simulate test web serve-web clean

all: build

build:
	$(CMAKE) -S . -B $(NATIVE_BUILD) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(NATIVE_BUILD) --parallel $(JOBS)

run: build
	./$(NATIVE_BUILD)/racest

agent: build
	$(PYTHON) src/agent/run.py

simulate: build
	$(PYTHON) src/agent/run.py --simulate --seed 42

test: build
	ctest --test-dir $(NATIVE_BUILD) --output-on-failure

# Activate emsdk_env.sh before building the WebAssembly version.
web:
	emcmake $(CMAKE) -S . -B $(WEB_BUILD) -DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(WEB_BUILD) --parallel $(JOBS)

serve-web: web
	$(PYTHON) src/agent/run.py --web

clean:
	rm -rf $(NATIVE_BUILD) $(WEB_BUILD)
