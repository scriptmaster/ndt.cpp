#!/bin/bash
# Setup script for GLFW on Windows with MinGW

echo "Setting up GLFW for Windows MinGW..."

# Create directories
mkdir -p include lib bin

# Check if GLFW is already installed locally
if [ -d "include/GLFW" ]; then
    echo "GLFW headers already found in include/GLFW"
    exit 0
fi

# Download GLFW pre-built binaries for MinGW64
echo "Downloading GLFW 3.3.8 for MinGW64..."

# Check for curl or wget
if command -v curl &> /dev/null; then
    DOWNLOAD_CMD="curl -L"
elif command -v wget &> /dev/null; then
    DOWNLOAD_CMD="wget -O-"
else
    echo "Error: Neither curl nor wget found. Please install one of them."
    exit 1
fi

# Try to download GLFW
GLFW_URL="https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.bin.WIN64.zip"
TEMP_FILE="glfw_temp.zip"

if $DOWNLOAD_CMD "$GLFW_URL" -o "$TEMP_FILE"; then
    echo "Extracting GLFW..."
    
    # Check for unzip
    if command -v unzip &> /dev/null; then
        unzip -q "$TEMP_FILE" -d glfw_temp || {
            echo "Error: Failed to extract GLFW. Please extract manually:"
            echo "  1. Download from: $GLFW_URL"
            echo "  2. Extract to ./include and ./lib directories"
            rm -f "$TEMP_FILE"
            exit 1
        }
        
        # Move files to correct locations
        if [ -d "glfw_temp/glfw-3.3.8.bin.WIN64" ]; then
            # Copy headers
            cp -r glfw_temp/glfw-3.3.8.bin.WIN64/include/GLFW include/ 2>/dev/null || true
            # Copy libs
            cp glfw_temp/glfw-3.3.8.bin.WIN64/lib-mingw-w64/*.a lib/ 2>/dev/null || true
            cp glfw_temp/glfw-3.3.8.bin.WIN64/lib-mingw-w64/*.dll bin/ 2>/dev/null || true
            
            # Cleanup
            rm -rf glfw_temp
            rm -f "$TEMP_FILE"
            
            echo "GLFW setup complete!"
            echo "Headers: ./include/GLFW"
            echo "Libraries: ./lib"
            echo "DLLs: ./bin"
        else
            echo "Error: Unexpected archive structure. Please install GLFW manually."
            rm -rf glfw_temp
            rm -f "$TEMP_FILE"
            exit 1
        fi
    else
        echo "Error: unzip not found. Please install unzip or extract manually."
        echo "Downloaded to: $TEMP_FILE"
        exit 1
    fi
else
    echo "Error: Failed to download GLFW."
    echo ""
    echo "Manual installation:"
    echo "1. Download GLFW from: https://www.glfw.org/download.html"
    echo "2. Extract the MinGW64 pre-built binaries"
    echo "3. Copy include/GLFW to ./include/GLFW"
    echo "4. Copy lib-mingw-w64/*.a to ./lib/"
    echo "5. Copy lib-mingw-w64/*.dll to ./bin/"
    exit 1
fi
