#!/bin/bash

# QlockThree Build Script
# This script helps with local development and testing

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    print_error "platformio.ini not found. Make sure you're in the project root directory."
    exit 1
fi

# Create config.h if it doesn't exist
if [ ! -f "include/config.h" ]; then
    print_warning "include/config.h not found. Creating from template..."
    cp include/config.h.template include/config.h
    print_success "Created include/config.h from template"
    print_warning "Remember to update WiFi credentials in include/config.h or leave empty for captive portal"
fi

# Parse command line arguments
COMMAND=${1:-build}
ENVIRONMENT=${2:-esp32-c3-devkitc-02}

case $COMMAND in
    "build")
        print_status "Building firmware for $ENVIRONMENT..."
        pio run --environment $ENVIRONMENT
        print_success "Build completed successfully!"
        
        # Show firmware info
        if [ -f ".pio/build/$ENVIRONMENT/firmware.bin" ]; then
            SIZE=$(stat -f%z ".pio/build/$ENVIRONMENT/firmware.bin" 2>/dev/null || stat -c%s ".pio/build/$ENVIRONMENT/firmware.bin" 2>/dev/null)
            print_status "Firmware size: $SIZE bytes"
            
            # Get version from config
            if [ -f "include/config.h" ]; then
                VERSION=$(grep 'CURRENT_VERSION' include/config.h | cut -d'"' -f2)
                print_status "Firmware version: $VERSION"
            fi
        fi
        ;;
        
    "upload")
        print_status "Building and uploading firmware to $ENVIRONMENT..."
        pio run --target upload --environment $ENVIRONMENT
        print_success "Upload completed successfully!"
        ;;
        
    "monitor")
        print_status "Opening serial monitor..."
        pio device monitor --environment $ENVIRONMENT
        ;;
        
    "upload-monitor" | "um")
        print_status "Building, uploading, and monitoring..."
        pio run --target upload --target monitor --environment $ENVIRONMENT
        ;;
        
    "clean")
        print_status "Cleaning build files..."
        pio run --target clean --environment $ENVIRONMENT
        print_success "Clean completed!"
        ;;
        
    "release")
        VERSION=${2}
        if [ -z "$VERSION" ]; then
            print_error "Version number required for release build"
            echo "Usage: $0 release VERSION"
            echo "Example: $0 release 1.0.1"
            exit 1
        fi
        
        print_status "Creating release build for version $VERSION..."
        
        # Update version in config
        sed -i.bak "s/const char\* CURRENT_VERSION = \".*\";/const char* CURRENT_VERSION = \"$VERSION\";/" include/config.h
        
        # Build firmware
        pio run --environment $ENVIRONMENT
        
        # Create release directory
        mkdir -p release
        
        # Copy firmware files
        cp .pio/build/$ENVIRONMENT/firmware.bin release/qlockthree-esp32c3-$VERSION.bin
        cp .pio/build/$ENVIRONMENT/bootloader.bin release/
        cp .pio/build/$ENVIRONMENT/partitions.bin release/
        
        # Create combined firmware if esptool is available
        if command -v esptool.py &> /dev/null; then
            print_status "Creating combined firmware..."
            esptool.py --chip esp32c3 merge_bin -o release/qlockthree-esp32c3-$VERSION-complete.bin \
                0x0 release/bootloader.bin \
                0x8000 release/partitions.bin \
                0x10000 release/qlockthree-esp32c3-$VERSION.bin
        else
            print_warning "esptool.py not found. Skipping combined firmware creation."
        fi
        
        # Create checksums
        cd release
        if command -v sha256sum &> /dev/null; then
            sha256sum *.bin > checksums.txt
        elif command -v shasum &> /dev/null; then
            shasum -a 256 *.bin > checksums.txt
        fi
        cd ..
        
        # Restore original config
        mv include/config.h.bak include/config.h
        
        print_success "Release build completed! Files in ./release/"
        ;;
        
    "help" | "-h" | "--help")
        echo "QlockThree Build Script"
        echo ""
        echo "Usage: $0 COMMAND [ARGS]"
        echo ""
        echo "Commands:"
        echo "  build                 Build firmware"
        echo "  upload                Build and upload firmware"
        echo "  monitor               Open serial monitor"
        echo "  upload-monitor (um)   Build, upload, and monitor"
        echo "  clean                 Clean build files"
        echo "  release VERSION       Create release build"
        echo "  help                  Show this help"
        echo ""
        echo "Examples:"
        echo "  $0 build              # Build firmware"
        echo "  $0 upload             # Upload to device"
        echo "  $0 um                 # Upload and monitor"
        echo "  $0 release 1.0.1      # Create release build"
        ;;
        
    *)
        print_error "Unknown command: $COMMAND"
        echo "Use '$0 help' for available commands"
        exit 1
        ;;
esac
