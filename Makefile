CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS =

# Platform-specific flags
UNAME_S := $(shell uname -s)
MINGW_DETECT := $(shell echo $(UNAME_S) | grep -i mingw || echo "")
MSYS_DETECT := $(shell echo $(UNAME_S) | grep -i msys || echo "")

ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lglfw -lGL -lm -ldl -lX11 -lpthread
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -framework Cocoa -framework IOKit -framework CoreVideo
    CXXFLAGS += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib
endif
ifeq ($(OS),Windows_NT)
    IS_WIN := 1
endif
ifneq ($(MINGW_DETECT),)
    IS_WIN := 1
endif
ifneq ($(MSYS_DETECT),)
    IS_WIN := 1
endif
ifeq ($(IS_WIN),1)
    # Try to find GLFW in common Windows/MinGW locations
    ifeq ($(shell test -d /c/ProgramData/mingw64/mingw64/include/GLFW && echo found),found)
        CXXFLAGS += -I/c/ProgramData/mingw64/mingw64/include
        LDFLAGS += -L/c/ProgramData/mingw64/mingw64/lib
    else ifeq ($(shell test -d /mingw64/include/GLFW && echo found),found)
        CXXFLAGS += -I/mingw64/include
        LDFLAGS += -L/mingw64/lib
    else ifeq ($(shell test -d ./include/GLFW && echo found),found)
        CXXFLAGS += -I./include
        LDFLAGS += -L./lib
    else ifeq ($(shell test -d ../include/GLFW && echo found),found)
        CXXFLAGS += -I../include
        LDFLAGS += -L../lib
    endif
    # Fully static linking - bakes everything into the exe except Windows system DLLs
    # Use -static for all libraries to avoid DLL dependencies
    LDFLAGS += -static -Wl,-Bstatic -lglfw3 -lwinpthread -Wl,-Bdynamic -lopengl32 -lgdi32 -lws2_32 -lwinmm -mwindows
    # Note: OpenGL32 and GDI32 are Windows system DLLs (always available on Windows)
endif

# Show configuration info
TARGET = ndt_display
SRCS = main.cpp display/logging.cpp display/window.cpp display/texture.cpp display/scene.cpp display/audio.cpp display/admin.cpp display/app.cpp display/render.cpp display/network.cpp display/scene_logger.cpp display/opening_scene.cpp
OBJS = $(SRCS:.cpp=.o)

# Add display directory to include path
CXXFLAGS += -Idisplay

all: $(TARGET) run

# Extract and increment version
VERSION_MAJOR := $(shell grep "define VERSION_MAJOR" version.h | awk '{print $$3}')
VERSION_MINOR := $(shell grep "define VERSION_MINOR" version.h | awk '{print $$3}')
VERSION_PATCH := $(shell grep "define VERSION_PATCH" version.h | awk '{print $$3}')
VERSION_BUILD := $(shell grep "define VERSION_BUILD" version.h | awk '{print $$3}')
VERSION_BUILD_NEW := $(shell echo $$(( $(VERSION_BUILD) + 1 )))
VERSION_STRING := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_BUILD_NEW)

# Vet must pass before version bump
vet-pass:
	@echo "Running code quality checks (vet) before version bump..."
	@$(MAKE) vet || (echo "Error: Code quality checks failed! Version will not be bumped." && exit 1)
	@echo "Success: Code quality checks passed. Proceeding with version bump..."

# Update version.h before build (only after vet passes)
version.h: vet-pass FORCE
	@echo "Incrementing version: $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_BUILD) -> $(VERSION_STRING)"
	@echo "#ifndef VERSION_H" > version.h
	@echo "#define VERSION_H" >> version.h
	@echo "" >> version.h
	@echo "#define VERSION_MAJOR $(VERSION_MAJOR)" >> version.h
	@echo "#define VERSION_MINOR $(VERSION_MINOR)" >> version.h
	@echo "#define VERSION_PATCH $(VERSION_PATCH)" >> version.h
	@echo "#define VERSION_BUILD $(VERSION_BUILD_NEW)" >> version.h
	@echo "" >> version.h
	@echo "#define VERSION_STRING \"$(VERSION_STRING)\"" >> version.h
	@echo "" >> version.h
	@echo "#endif // VERSION_H" >> version.h

build: version.h $(OBJS)
	@mkdir -p bin
	@mkdir -p bin/builds/builds config
	@if [ ! -f config/audio_seed.txt ]; then \
		echo "12345" > config/audio_seed.txt; \
		echo "Created config/audio_seed.txt with default seed"; \
	fi
	@echo "Building $(TARGET) v$(VERSION_STRING) (Platform: $(UNAME_S))"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "--------------------------------"
	@BUILD_TARGET=$(TARGET).v$(VERSION_STRING).exe; \
	if [ -f $(TARGET).exe ]; then \
		echo "Building to versioned file first: $$BUILD_TARGET (to avoid locking $(TARGET).exe)"; \
		$(CXX) $(OBJS) -o $$BUILD_TARGET $(LDFLAGS); \
	else \
		echo "Building to: $(TARGET).exe"; \
		BUILD_TARGET=$(TARGET).exe; \
		$(CXX) $(OBJS) -o $$BUILD_TARGET $(LDFLAGS); \
	fi; \
	if [ $$? -eq 0 ]; then \
		if [ -f $$BUILD_TARGET ]; then \
			SIZE=$$(ls -lh "$$BUILD_TARGET" 2>/dev/null | awk '{print $$5}'); \
			echo "Success: Built $$BUILD_TARGET (Size: $$SIZE)"; \
			if [ "$$BUILD_TARGET" != "$(TARGET).exe" ]; then \
				# Try to release/close the running executable if it's locked \
				if [ -f $(TARGET).exe ]; then \
					echo "Checking if $(TARGET).exe is running..."; \
					if command -v taskkill > /dev/null 2>&1; then \
						taskkill //F //IM $(TARGET).exe //T > /dev/null 2>&1 && echo "Closed running $(TARGET).exe process" || echo "No running process found or already closed"; \
					elif command -v pkill > /dev/null 2>&1; then \
						pkill -f $(TARGET) && echo "Closed running $(TARGET) process" || echo "No running process found or already closed"; \
					fi; \
					sleep 1; \
				fi; \
				cp $$BUILD_TARGET $(TARGET).exe 2>/dev/null && echo "Success: Copied to $(TARGET).exe" || echo "Warning: Could not copy to $(TARGET).exe (may still be locked)"; \
				mv $$BUILD_TARGET bin/builds/builds/v$(VERSION_STRING).exe && \
				echo "Success: Versioned binary created at bin/builds/builds/v$(VERSION_STRING).exe"; \
			else \
				cp $(TARGET).exe bin/builds/builds/v$(VERSION_STRING).exe && \
				echo "Success: Versioned binary created at bin/builds/builds/v$(VERSION_STRING).exe"; \
			fi; \
		fi; \
	else \
		echo "Error: Build failed!"; \
		exit 1; \
	fi
	@if [ -f bin/builds/builds/v$(VERSION_STRING).exe ]; then \
		echo "Bundling required DLLs..."; \
		if command -v objdump > /dev/null 2>&1; then \
			for DLL in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll glfw3.dll; do \
				DLL_PATH=$$(find /mingw64/bin /usr/bin /bin -name "$$DLL" 2>/dev/null | head -1); \
				if [ -n "$$DLL_PATH" ] && [ -f "$$DLL_PATH" ]; then \
					cp "$$DLL_PATH" bin/builds/builds/$$DLL 2>/dev/null || true; \
					echo "  Bundled: $$DLL"; \
				fi; \
			done; \
		fi; \
		\
		# Create/update run.cmd with latest version (simple 2-line format) \
		echo "git pull" > bin/builds/builds/run.cmd; \
		echo "v$(VERSION_STRING).exe" >> bin/builds/builds/run.cmd; \
		\
		echo "Committing and pushing build v$(VERSION_STRING)..."; \
		cd bin/builds && git add builds/v$(VERSION_STRING).exe builds/*.dll builds/run.cmd 2>/dev/null || true && \
		git commit -m "Add build v$(VERSION_STRING)" && git push || echo "Git push skipped or failed"; \
		echo "Success: Build v$(VERSION_STRING) completed and pushed!"; \
	elif [ -f bin/builds/builds/v$(VERSION_STRING) ]; then \
		echo "Committing and pushing build v$(VERSION_STRING)..."; \
		cd bin/builds && git add builds/v$(VERSION_STRING) && git commit -m "Add build v$(VERSION_STRING)" && git push || echo "Git push skipped or failed"; \
		echo "Success: Build v$(VERSION_STRING) completed and pushed!"; \
	fi

$(TARGET): build
	@EXE_FILE=""; \
	if [ -f $(TARGET).exe ]; then \
		EXE_FILE="$(TARGET).exe"; \
	elif [ -f $(TARGET) ]; then \
		EXE_FILE="$(TARGET)"; \
	fi; \
	if [ -n "$$EXE_FILE" ]; then \
		SIZE=$$(ls -lh "$$EXE_FILE" 2>/dev/null | awk '{print $$5}'); \
		echo "Success: $$EXE_FILE created successfully. Size: $$SIZE bytes"; \
	else \
		echo "Failed: $(TARGET) not created"; \
		exit 1; \
	fi;

run:
	@echo "Running latest versioned executable"
	@LATEST_VERSION=$$(grep "define VERSION_BUILD" version.h | awk '{print $$3}'); \
	VERSION_MAJOR=$$(grep "define VERSION_MAJOR" version.h | awk '{print $$3}'); \
	VERSION_MINOR=$$(grep "define VERSION_MINOR" version.h | awk '{print $$3}'); \
	VERSION_PATCH=$$(grep "define VERSION_PATCH" version.h | awk '{print $$3}'); \
	VERSION_STRING="$$VERSION_MAJOR.$$VERSION_MINOR.$$VERSION_PATCH.$$LATEST_VERSION"; \
	VERSIONED_EXE="$(TARGET).v$$VERSION_STRING.exe"; \
	VERSIONED_EXE_BUILDS="bin/builds/builds/v$$VERSION_STRING.exe"; \
	if [ -f "$$VERSIONED_EXE" ]; then \
		echo "Running $$VERSIONED_EXE"; \
		./$$VERSIONED_EXE; EXIT_CODE=$$?; \
	elif [ -f "$$VERSIONED_EXE_BUILDS" ]; then \
		echo "Running $$VERSIONED_EXE_BUILDS"; \
		./$$VERSIONED_EXE_BUILDS; EXIT_CODE=$$?; \
	elif [ -f "$(TARGET).exe" ]; then \
		echo "Versioned executable not found, running $(TARGET).exe"; \
		./$(TARGET).exe; EXIT_CODE=$$?; \
	elif [ -f "$(TARGET)" ]; then \
		echo "Versioned executable not found, running $(TARGET)"; \
		./$(TARGET); EXIT_CODE=$$?; \
	else \
		echo "Error: No executable found to run"; \
		exit 1; \
	fi; \
	if [ $$EXIT_CODE -eq 0 ]; then \
		echo "NDT Logo Display shut down gracefully"; \
	elif [ $$EXIT_CODE -eq 139 ]; then \
		echo "NDT Logo Display exited (suppressed exit code 139)"; \
		exit 0; \
	else \
		echo "NDT Logo Display exited with code $$EXIT_CODE"; \
		exit $$EXIT_CODE; \
	fi

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Vet target - check code quality
vet:
	@echo "Running code quality checks..."
	@ERRORS=0; \
	for file in main.cpp display/*.cpp display/*.h; do \
		if [ -f "$$file" ]; then \
			echo "Checking $$file..."; \
			$(CXX) $(CXXFLAGS) -c "$$file" -o /dev/null 2>&1 | grep -i "error" && ERRORS=$$((ERRORS + 1)) || true; \
		fi; \
	done; \
	if [ $$ERRORS -gt 0 ]; then \
		echo "Vet found $$ERRORS error(s)"; \
		exit 1; \
	else \
		echo "Vet passed: No errors found"; \
	fi

# Test target - similar to go test
TEST_TARGET = test_runner
TEST_SRCS = test/test_main.cpp test/test_register.cpp test/scene_test.cpp test/audio_test.cpp test/color_test.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

test: $(TEST_TARGET)
	@echo "Running tests..."
	@./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) display/scene.o display/audio.o display/logging.o
	@echo "Building test runner..."
	@# For tests, link OpenGL libraries (scene.cpp uses OpenGL functions)
	@# Use -Wl,--whole-archive to ensure all symbols are included (helps with static initialization)
	@if [ -f display/scene.o ] && [ -f display/audio.o ] && [ -f display/logging.o ]; then \
		$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) display/scene.o display/audio.o display/logging.o -L./lib -Wl,--whole-archive -static-libgcc -static-libstdc++ -Wl,--no-whole-archive -lopengl32 -lgdi32; \
	else \
		$(CXX) $(CXXFLAGS) -c display/scene.cpp -o display/scene.o; \
		$(CXX) $(CXXFLAGS) -c display/audio.cpp -o display/audio.o; \
		$(CXX) $(CXXFLAGS) -c display/logging.cpp -o display/logging.o; \
		$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) display/scene.o display/audio.o display/logging.o -L./lib -Wl,--whole-archive -static-libgcc -static-libstdc++ -Wl,--no-whole-archive -lopengl32 -lgdi32; \
	fi
	@echo "Test runner built successfully"

test/%.o: test/%.cpp test/test.h
	@mkdir -p test
	$(CXX) $(CXXFLAGS) -Itest -I. -Idisplay -c $< -o $@

clean-test:
	rm -f $(TEST_OBJS) $(TEST_TARGET) test_*.json test_*.txt

clean: clean-test

clean:
	rm -f $(OBJS) $(TARGET)

# Windows-specific: Setup GLFW
setup-glfw:
	@echo "Setting up GLFW for Windows..."
	@if command -v powershell > /dev/null 2>&1; then \
		powershell -ExecutionPolicy Bypass -File setup_glfw.ps1; \
	elif command -v cmd.exe > /dev/null 2>&1; then \
		cmd.exe /c setup_glfw.bat; \
	else \
		bash setup_glfw.sh; \
	fi

FORCE:

# MSI packaging
msi: $(TARGET)
	@echo "Building MSI installer v$(VERSION_STRING)..."
	@mkdir -p wix
	@mkdir -p bin/builds/builds
	@\
	# Check for WiX toolset \
	if ! command -v candle.exe > /dev/null 2>&1 && ! command -v candle > /dev/null 2>&1; then \
		echo "WiX Toolset not found. Checking for winget..."; \
		if command -v winget > /dev/null 2>&1; then \
			echo "Installing WiX Toolset via winget..."; \
			winget install -e --id WiXToolset.WiXToolset --silent --accept-package-agreements --accept-source-agreements || \
			echo "Failed to install WiX Toolset. Please install manually: winget install -e --id WiXToolset.WiXToolset"; \
		else \
			echo "Error: WiX Toolset not found and winget not available."; \
			echo "Please install WiX Toolset manually from: https://wixtoolset.org/"; \
			exit 1; \
		fi; \
	fi; \
	\
	# Find WiX binaries \
	WIXBIN=""; \
	if command -v candle.exe > /dev/null 2>&1; then \
		WIXBIN=$$(dirname "$$(which candle.exe)"); \
	elif command -v candle > /dev/null 2>&1; then \
		WIXBIN=$$(dirname "$$(which candle)"); \
	else \
		# Try common WiX installation paths \
		for WIXPATH in "/c/Program Files (x86)/WiX Toolset v*/bin" "/c/Program Files/WiX Toolset v*/bin" "C:\\Program Files (x86)\\WiX Toolset v*\\bin" "C:\\Program Files\\WiX Toolset v*\\bin"; do \
			if [ -d "$$WIXPATH" ] && [ -f "$$WIXPATH/candle.exe" ]; then \
				WIXBIN="$$WIXPATH"; \
				break; \
			fi; \
		done; \
	fi; \
	\
	if [ -z "$$WIXBIN" ] || [ ! -f "$$WIXBIN/candle.exe" ] && [ ! -f "$$WIXBIN/candle" ]; then \
		echo "Error: WiX Toolset (candle) not found in PATH or common locations."; \
		echo "Please ensure WiX Toolset is installed and in PATH."; \
		exit 1; \
	fi; \
	\
	# Prepare build directory with all files \
	BUILDDIR="wix/build"; \
	mkdir -p "$$BUILDDIR"; \
	\
	# Copy executable and DLLs to build directory \
	if [ -f $(TARGET).exe ]; then \
		cp $(TARGET).exe "$$BUILDDIR/$(TARGET).exe"; \
		for DLL in glfw3.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do \
			if [ -f "bin/$$DLL" ]; then \
				cp "bin/$$DLL" "$$BUILDDIR/$$DLL"; \
			elif [ -f "bin/builds/builds/$$DLL" ]; then \
				cp "bin/builds/builds/$$DLL" "$$BUILDDIR/$$DLL"; \
			fi; \
		done; \
	else \
		echo "Error: $(TARGET).exe not found. Please build first with 'make build'"; \
		exit 1; \
	fi; \
	\
	# Read current version from version.h \
	CURRENT_VERSION=$$(grep "define VERSION_STRING" version.h | awk '{print $$3}' | tr -d '"'); \
	\
	# Update version in WXS file \
	sed "s/\$(var.ProductVersion)/$$CURRENT_VERSION/g" wix/installer.wxs > "$$BUILDDIR/installer.wxs"; \
	\
	# Build MSI using WiX \
	CANDLE="$$WIXBIN/candle.exe"; \
	if [ ! -f "$$CANDLE" ]; then \
		CANDLE="$$WIXBIN/candle"; \
	fi; \
	\
	LIGHT="$$WIXBIN/light.exe"; \
	if [ ! -f "$$LIGHT" ]; then \
		LIGHT="$$WIXBIN/light"; \
	fi; \
	\
	echo "Compiling WiX source files (version: $$CURRENT_VERSION)..."; \
	"$$CANDLE" -dBinPath="$$BUILDDIR" -dTargetName="$(TARGET)" -dProductVersion="$$CURRENT_VERSION" "$$BUILDDIR/installer.wxs" -out "$$BUILDDIR/" || { \
		echo "Error: WiX candle failed"; \
		exit 1; \
	}; \
	\
	echo "Linking MSI package..."; \
	"$$LIGHT" "$$BUILDDIR/installer.wixobj" -out "bin/builds/builds/app-v$$CURRENT_VERSION.msi" -ext WixUIExtension || { \
		echo "Error: WiX light failed"; \
		exit 1; \
	}; \
	\
	echo "MSI installer created: bin/builds/builds/app-v$$CURRENT_VERSION.msi"; \
	rm -rf "$$BUILDDIR"

# Zip target - create a downloadable zip of the entire codebase
zip:
	@echo "Creating codebase zip file..."
	@if [ ! -f version.h ]; then \
		echo "Error: version.h not found. Please ensure you're in the project root directory."; \
		exit 1; \
	fi; \
	VERSION_STRING=$$(sed -n 's/.*#define VERSION_STRING \"\([^\"]*\)\".*/\1/p' version.h); \
	if [ -z "$$VERSION_STRING" ]; then \
		echo "Error: Could not extract version from version.h"; \
		exit 1; \
	fi; \
	ZIP_NAME="ndt.cpp-v$$VERSION_STRING.zip"; \
	echo "Creating $$ZIP_NAME..."; \
	if command -v zip > /dev/null 2>&1; then \
		zip -r "$$ZIP_NAME" . \
			-x "*.o" \
			-x "*.exe" \
			-x "*.msi" \
			-x "*.wixobj" \
			-x "*.wixpdb" \
			-x "*.log" \
			-x ".git/*" \
			-x "bin/*" \
			-x "builds/*" \
			-x "wix/build/*" \
			-x "test_runner" \
			-x "test_runner.exe" \
			-x "$(TARGET)" \
			-x "$(TARGET).exe" \
			-x "$(TARGET).v*.exe" \
			-x "ndt.cpp-v*.zip"; \
		echo "Success: Created $$ZIP_NAME"; \
		ls -lh "$$ZIP_NAME"; \
	else \
		echo "Error: 'zip' command not found."; \
		echo "Please install the zip utility:"; \
		echo "  - Ubuntu/Debian: sudo apt-get install zip"; \
		echo "  - macOS: zip is pre-installed"; \
		echo "  - Windows/MSYS2: pacman -S zip"; \
		exit 1; \
	fi

.PHONY: all build clean setup-glfw msi vet zip FORCE
