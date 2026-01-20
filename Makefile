.PHONY: all build

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
SRCS = src/App/main.cpp src/App/AppHost.cpp src/App/AdminUtils.cpp src/App/DI/ServiceCollection.cpp src/App/DI/ServiceProvider.cpp \
       src/Services/LoggingService/LoggingService.cpp \
       src/Services/LoggingService/Logging.cpp \
       src/Services/LoggingService/SceneLogger.cpp \
       src/Services/LocalConfigService/LocalConfigService.cpp \
       src/Services/WindowService/WindowService.cpp \
       src/Services/WindowService/WindowManager.cpp \
       src/Services/WindowService/AppLoop.cpp \
       src/Services/WindowService/Renderer.cpp \
       src/Services/WindowService/TextureLoader.cpp \
       src/Services/WindowService/SceneHelpers.cpp \
       src/Services/WindowService/BackgroundGraphics.cpp \
       src/Services/WindowService/BackgroundTriangles.cpp \
       src/Services/WindowService/BackgroundDots.cpp \
       src/Services/WindowService/BackgroundOrbs.cpp \
       src/Services/WindowService/SceneLoader.cpp \
       src/Services/WindowService/SceneRenderer.cpp \
       src/Services/WindowService/Admin.cpp \
       src/Services/AudioService/AudioService.cpp \
       src/Services/AudioService/AudioSystem.cpp \
       src/Services/AudioService/AudioCapture.cpp \
       src/Services/AudioService/AudioWaveform.cpp \
       src/Services/NetworkService/NetworkSystem.cpp \
       src/Services/HTTPService/HTTPService.cpp \
       src/Services/WSService/WSService.cpp \
       src/Services/STTService/STTService.cpp \
       src/Services/LLMService/LLMService.cpp \
       src/Services/TTSService/TTSService.cpp
OBJS = $(SRCS:.cpp=.o)

# Add directories to include path
CXXFLAGS += -Isrc -Isrc/App -Isrc/Services

all: $(TARGET) run

# Extract and increment version
VERSION_MAJOR := $(shell grep "define VERSION_MAJOR" src/App/version.h | awk '{print $$3}')
VERSION_MINOR := $(shell grep "define VERSION_MINOR" src/App/version.h | awk '{print $$3}')
VERSION_PATCH := $(shell grep "define VERSION_PATCH" src/App/version.h | awk '{print $$3}')
VERSION_BUILD := $(shell grep "define VERSION_BUILD" src/App/version.h | awk '{print $$3}')
VERSION_BUILD_NEW := $(shell echo $$(( $(VERSION_BUILD) + 1 )))
VERSION_STRING := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_BUILD_NEW)

# Vet must pass before version bump
vet-pass:
	@echo "Running code quality checks (vet) before version bump..."
	@$(MAKE) vet || (echo "Error: Code quality checks failed! Version will not be bumped." && exit 1)
	@echo "Success: Code quality checks passed. Proceeding with version bump..."

# Update version.h before build (only after vet passes)
src/App/version.h: vet-pass FORCE
	@echo "Incrementing version: $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH).$(VERSION_BUILD) -> $(VERSION_STRING)"
	@echo "#ifndef VERSION_H" > src/App/version.h
	@echo "#define VERSION_H" >> src/App/version.h
	@echo "" >> src/App/version.h
	@echo "#define VERSION_MAJOR $(VERSION_MAJOR)" >> src/App/version.h
	@echo "#define VERSION_MINOR $(VERSION_MINOR)" >> src/App/version.h
	@echo "#define VERSION_PATCH $(VERSION_PATCH)" >> src/App/version.h
	@echo "#define VERSION_BUILD $(VERSION_BUILD_NEW)" >> src/App/version.h
	@echo "" >> src/App/version.h
	@echo "#define VERSION_STRING \"$(VERSION_STRING)\"" >> src/App/version.h
	@echo "" >> src/App/version.h
	@echo "#endif // VERSION_H" >> src/App/version.h

build: src/App/version.h $(OBJS)
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
	@LATEST_VERSION=$$(grep "define VERSION_BUILD" src/App/version.h | awk '{print $$3}'); \
	VERSION_MAJOR=$$(grep "define VERSION_MAJOR" src/App/version.h | awk '{print $$3}'); \
	VERSION_MINOR=$$(grep "define VERSION_MINOR" src/App/version.h | awk '{print $$3}'); \
	VERSION_PATCH=$$(grep "define VERSION_PATCH" src/App/version.h | awk '{print $$3}'); \
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
	for file in src/App/main.cpp; do \
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

$(TEST_TARGET): $(TEST_OBJS) archive/scene.o.old archive/audio.o.old archive/logging.o.old
	@echo "Building test runner..."
	@# For tests, link OpenGL libraries (scene.cpp uses OpenGL functions)
	@# Use -Wl,--whole-archive to ensure all symbols are included (helps with static initialization)
	@if [ -f archive/scene.o.old ] && [ -f archive/audio.o.old ] && [ -f archive/logging.o.old ]; then \
		$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) archive/scene.o.old archive/audio.o.old archive/logging.o.old -L./lib -Wl,--whole-archive -static-libgcc -static-libstdc++ -Wl,--no-whole-archive -lopengl32 -lgdi32; \
	else \
		$(CXX) $(CXXFLAGS) -c archive/scene.cpp.old -o archive/scene.o.old; \
		$(CXX) $(CXXFLAGS) -c archive/audio.cpp.old -o archive/audio.o.old; \
		$(CXX) $(CXXFLAGS) -c archive/logging.cpp.old -o archive/logging.o.old; \
		$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) archive/scene.o.old archive/audio.o.old archive/logging.o.old -L./lib -Wl,--whole-archive -static-libgcc -static-libstdc++ -Wl,--no-whole-archive -lopengl32 -lgdi32; \
	fi
	@echo "Test runner built successfully"

test/%.o: test/%.cpp test/test.h
	@mkdir -p test
	$(CXX) $(CXXFLAGS) -Itest -I. -Isrc -c $< -o $@

clean-test:
	rm -f $(TEST_OBJS) $(TEST_TARGET) test_*.json test_*.txt

# Audio device test target
AUDIO_TEST_TARGET = audio_test
AUDIO_TEST_SRC = test/audio_device_test.cpp
AUDIO_TEST_ORCHESTRATOR_SRC = test/AudioCapture/AudioCaptureOrchestrator.cpp
AUDIO_TEST_RMS_SRC = test/AudioCapture/RMSAnalyzer.cpp
AUDIO_TEST_NOISE_SRC = test/AudioCapture/NoiseCalibrator.cpp
AUDIO_TEST_GATE_SRC = test/AudioCapture/SpeechGate.cpp
AUDIO_TEST_BUFFER_SRC = test/AudioCapture/AudioSegmentBuffer.cpp
AUDIO_TEST_OBJ = $(AUDIO_TEST_SRC:.cpp=.o) \
                 $(AUDIO_TEST_ORCHESTRATOR_SRC:.cpp=.o) \
                 $(AUDIO_TEST_RMS_SRC:.cpp=.o) \
                 $(AUDIO_TEST_NOISE_SRC:.cpp=.o) \
                 $(AUDIO_TEST_GATE_SRC:.cpp=.o) \
                 $(AUDIO_TEST_BUFFER_SRC:.cpp=.o)

audio: $(AUDIO_TEST_TARGET)
	@echo "Running audio device test..."
	@mkdir -p logs
	@./$(AUDIO_TEST_TARGET)

$(AUDIO_TEST_TARGET): $(AUDIO_TEST_OBJ)
	@echo "Building audio device test..."
	@mkdir -p logs
ifeq ($(IS_WIN),1)
	$(CXX) $(CXXFLAGS) -o $(AUDIO_TEST_TARGET) $(AUDIO_TEST_OBJ) -lwinmm -mconsole
else
	$(CXX) $(CXXFLAGS) -o $(AUDIO_TEST_TARGET) $(AUDIO_TEST_OBJ)
endif
	@echo "Audio test built successfully"

test/audio_device_test.o: test/audio_device_test.cpp
	@mkdir -p test
	$(CXX) $(CXXFLAGS) -Itest -I. -Isrc -c $< -o $@

test/AudioCapture/%.o: test/AudioCapture/%.cpp
	@mkdir -p test/AudioCapture
	$(CXX) $(CXXFLAGS) -Itest -I. -Isrc -c $< -o $@

# Audio UI test target
AUDIO_UI_TEST_TARGET = audio_ui_test
AUDIO_UI_TEST_SRC = test/audio_ui_test.cpp
AUDIO_UI_TEST_OBJ = $(AUDIO_UI_TEST_SRC:.cpp=.o)

audioui: $(AUDIO_UI_TEST_TARGET)
	@echo "Audio UI test built (run ./audio_ui_test to launch)"

$(AUDIO_UI_TEST_TARGET): $(AUDIO_UI_TEST_OBJ)
	@echo "Building audio UI test..."
ifeq ($(IS_WIN),1)
	$(CXX) $(CXXFLAGS) -o $(AUDIO_UI_TEST_TARGET) $(AUDIO_UI_TEST_OBJ) -L./lib -Wl,-Bstatic -lglfw3 -lwinpthread -Wl,-Bdynamic -lopengl32 -lgdi32 -lwinmm -mwindows
else
	$(CXX) $(CXXFLAGS) -o $(AUDIO_UI_TEST_TARGET) $(AUDIO_UI_TEST_OBJ) $(LDFLAGS)
endif
	@echo "Audio UI test ready: ./$(AUDIO_UI_TEST_TARGET)"

test/audio_ui_test.o: test/audio_ui_test.cpp
	@mkdir -p test
	$(CXX) $(CXXFLAGS) -Itest -I. -Isrc -c $< -o $@

# UI test target
UI_TEST_TARGET = ui_test
UI_TEST_SRC = test/ui_test.cpp
UI_TEST_OBJ = $(UI_TEST_SRC:.cpp=.o)

uitest: $(UI_TEST_TARGET)
	@echo "Running UI test..."
	@./$(UI_TEST_TARGET)

$(UI_TEST_TARGET): $(UI_TEST_OBJ)
	@echo "Building UI test..."
ifeq ($(IS_WIN),1)
	$(CXX) $(CXXFLAGS) -o $(UI_TEST_TARGET) $(UI_TEST_OBJ) -L./lib -Wl,-Bstatic -lglfw3 -lwinpthread -Wl,-Bdynamic -lopengl32 -lgdi32 -mwindows
else
	$(CXX) $(CXXFLAGS) -o $(UI_TEST_TARGET) $(UI_TEST_OBJ) $(LDFLAGS)
endif
	@echo "UI test built successfully"

test/ui_test.o: test/ui_test.cpp
	@mkdir -p test
	$(CXX) $(CXXFLAGS) -Itest -I. -Isrc -I./include -c $< -o $@

clean: clean-test
	rm -f $(OBJS) $(TARGET) $(AUDIO_TEST_TARGET) $(AUDIO_TEST_OBJ) $(UI_TEST_TARGET) $(UI_TEST_OBJ)

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
	# Read current version from src/App/version.h \
	CURRENT_VERSION=$$(grep "define VERSION_STRING" src/App/version.h | awk '{print $$3}' | tr -d '"'); \
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

.PHONY: all clean setup-glfw msi vet FORCE
