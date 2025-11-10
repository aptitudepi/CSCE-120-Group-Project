# Makefile for Hyperlocal Weather Application
# Comprehensive build and test automation system

# Project configuration
PROJECT_NAME := HyperlocalWeather
PROJECT_VERSION := 1.0.0
BUILD_DIR := build
BUILD_DIR_DEBUG := build-debug
BUILD_DIR_RELEASE := build-release
BUILD_DIR_COVERAGE := build-coverage
BUILD_DIR_TEST := build-test

# Build system
CMAKE := cmake
NINJA := ninja
CTEST := ctest
CMAKE_GENERATOR := Ninja

# Build types
BUILD_TYPE_DEBUG := Debug
BUILD_TYPE_RELEASE := Release
BUILD_TYPE_RELWITHDEBINFO := RelWithDebInfo

# Default build type
BUILD_TYPE ?= Release

# Parallel build jobs (use all CPU cores by default)
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

# Colors for output
COLOR_RESET := \033[0m
COLOR_BOLD := \033[1m
COLOR_GREEN := \033[32m
COLOR_YELLOW := \033[33m
COLOR_RED := \033[31m
COLOR_BLUE := \033[34m

# Default target
.DEFAULT_GOAL := help

# Phony targets
.PHONY: all build clean test install help format lint coverage docs check deps

#=============================================================================
# Help Target - Shows all available targets
#=============================================================================

help: ## Show this help message
	@echo "$(COLOR_BOLD)Hyperlocal Weather - Build System$(COLOR_RESET)"
	@echo ""
	@echo "$(COLOR_BOLD)Available targets:$(COLOR_RESET)"
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  $(COLOR_GREEN)%-20s$(COLOR_RESET) %s\n", $$1, $$2}'
	@echo ""
	@echo "$(COLOR_BOLD)Examples:$(COLOR_RESET)"
	@echo "  make build              # Build release version"
	@echo "  make build-debug        # Build debug version"
	@echo "  make test               # Run all tests"
	@echo "  make coverage           # Generate coverage report"
	@echo "  make clean              # Clean build artifacts"
	@echo ""

#=============================================================================
# Dependency Checking
#=============================================================================

check-deps: ## Check if all required dependencies are installed
	@echo "$(COLOR_BLUE)Checking dependencies...$(COLOR_RESET)"
	@command -v $(CMAKE) >/dev/null 2>&1 || { echo "$(COLOR_RED)Error: cmake not found$(COLOR_RESET)"; exit 1; }
	@command -v $(NINJA) >/dev/null 2>&1 || { echo "$(COLOR_RED)Error: ninja not found$(COLOR_RESET)"; exit 1; }
	@command -v qmake6 >/dev/null 2>&1 || { echo "$(COLOR_YELLOW)Warning: qmake6 not found. Qt6 may not be installed.$(COLOR_RESET)"; }
	@echo "$(COLOR_GREEN)✓ All required dependencies found$(COLOR_RESET)"

install-deps: ## Install build dependencies (Ubuntu/Debian)
	@echo "$(COLOR_BLUE)Installing dependencies...$(COLOR_RESET)"
	sudo apt update
	sudo apt install -y \
		build-essential \
		cmake \
		ninja-build \
		qt6-base-dev \
		qt6-declarative-dev \
		qt6-tools-dev \
		qml6-module-qtquick \
		qml6-module-qtquick-controls \
		qml6-module-qtquick-layouts \
		qml6-module-qtquick-window \
		libqt6sql6-sqlite \
		libsqlite3-dev \
		libgtest-dev \
		pkg-config \
		patchelf
	@echo "$(COLOR_GREEN)✓ Dependencies installed$(COLOR_RESET)"

#=============================================================================
# Build Targets
#=============================================================================

all: build ## Build the project (default: release)

build: check-deps ## Build release version
	@echo "$(COLOR_BLUE)Building $(PROJECT_NAME) (Release)...$(COLOR_RESET)"
	@mkdir -p $(BUILD_DIR_RELEASE)
	@cd $(BUILD_DIR_RELEASE) && \
		$(CMAKE) -G $(CMAKE_GENERATOR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_RELEASE) \
			.. && \
		$(NINJA) -j$(JOBS)
	@echo "$(COLOR_GREEN)✓ Build complete: $(BUILD_DIR_RELEASE)/bin/$(PROJECT_NAME)$(COLOR_RESET)"

build-debug: check-deps ## Build debug version
	@echo "$(COLOR_BLUE)Building $(PROJECT_NAME) (Debug)...$(COLOR_RESET)"
	@mkdir -p $(BUILD_DIR_DEBUG)
	@cd $(BUILD_DIR_DEBUG) && \
		$(CMAKE) -G $(CMAKE_GENERATOR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_DEBUG) \
			.. && \
		$(NINJA) -j$(JOBS)
	@echo "$(COLOR_GREEN)✓ Debug build complete: $(BUILD_DIR_DEBUG)/bin/$(PROJECT_NAME)$(COLOR_RESET)"

build-test: check-deps ## Build with tests enabled
	@echo "$(COLOR_BLUE)Building $(PROJECT_NAME) with tests...$(COLOR_RESET)"
	@mkdir -p $(BUILD_DIR_TEST)
	@cd $(BUILD_DIR_TEST) && \
		$(CMAKE) -G $(CMAKE_GENERATOR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_DEBUG) \
			-DBUILD_TESTS=ON \
			.. && \
		$(NINJA) -j$(JOBS)
	@echo "$(COLOR_GREEN)✓ Test build complete$(COLOR_RESET)"

build-coverage: check-deps ## Build with coverage instrumentation
	@echo "$(COLOR_BLUE)Building $(PROJECT_NAME) with coverage...$(COLOR_RESET)"
	@command -v lcov >/dev/null 2>&1 || { \
		echo "$(COLOR_YELLOW)Installing coverage tools...$(COLOR_RESET)"; \
		sudo apt install -y lcov gcovr; \
	}
	@mkdir -p $(BUILD_DIR_COVERAGE)
	@cd $(BUILD_DIR_COVERAGE) && \
		$(CMAKE) -G $(CMAKE_GENERATOR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_DEBUG) \
			-DENABLE_COVERAGE=ON \
			.. && \
		$(NINJA) -j$(JOBS)
	@echo "$(COLOR_GREEN)✓ Coverage build complete$(COLOR_RESET)"

rebuild: clean build ## Clean and rebuild from scratch

rebuild-debug: clean build-debug ## Clean and rebuild debug version

#=============================================================================
# Test Targets
#=============================================================================

test: build-test ## Run all tests
	@echo "$(COLOR_BLUE)Running all tests...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		$(CTEST) --output-on-failure -V
	@echo "$(COLOR_GREEN)✓ Tests complete$(COLOR_RESET)"

test-unit: build-test ## Run unit tests only
	@echo "$(COLOR_BLUE)Running unit tests...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		./tests/HyperlocalWeatherTests --gtest_filter="*Test.*:*Test_*" \
			--gtest_color=yes
	@echo "$(COLOR_GREEN)✓ Unit tests complete$(COLOR_RESET)"

test-integration: build-test ## Run integration tests only
	@echo "$(COLOR_BLUE)Running integration tests...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		./tests/HyperlocalWeatherTests --gtest_filter="*Integration*:*EndToEnd*" \
			--gtest_color=yes
	@echo "$(COLOR_GREEN)✓ Integration tests complete$(COLOR_RESET)"

test-services: build-test ## Run service tests only
	@echo "$(COLOR_BLUE)Running service tests...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		./tests/HyperlocalWeatherTests --gtest_filter="*Service*" \
			--gtest_color=yes
	@echo "$(COLOR_GREEN)✓ Service tests complete$(COLOR_RESET)"

test-quick: build-test ## Run quick tests (no network)
	@echo "$(COLOR_BLUE)Running quick tests (no network)...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		./tests/HyperlocalWeatherTests --gtest_filter="*Test.*:*Test_*" \
			--gtest_color=yes
	@echo "$(COLOR_GREEN)✓ Quick tests complete$(COLOR_RESET)"

test-verbose: build-test ## Run tests with verbose output
	@echo "$(COLOR_BLUE)Running tests with verbose output...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		./tests/HyperlocalWeatherTests --gtest_color=yes --gtest_print_time=1
	@echo "$(COLOR_GREEN)✓ Verbose tests complete$(COLOR_RESET)"

#=============================================================================
# Coverage Targets
#=============================================================================

coverage: build-coverage ## Generate code coverage report
	@echo "$(COLOR_BLUE)Generating coverage report...$(COLOR_RESET)"
	@cd $(BUILD_DIR_COVERAGE) && \
		export QT_QPA_PLATFORM=offscreen && \
		$(CTEST) --output-on-failure && \
		$(NINJA) coverage 2>/dev/null || \
		(echo "$(COLOR_YELLOW)Coverage target not available, generating manually...$(COLOR_RESET)" && \
		 lcov --capture --directory . --output-file coverage.info && \
		 lcov --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' --output-file coverage.info && \
		 genhtml coverage.info --output-directory coverage)
	@echo "$(COLOR_GREEN)✓ Coverage report generated: $(BUILD_DIR_COVERAGE)/coverage/index.html$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)Open with: xdg-open $(BUILD_DIR_COVERAGE)/coverage/index.html$(COLOR_RESET)"

coverage-summary: build-coverage ## Show coverage summary
	@cd $(BUILD_DIR_COVERAGE) && \
		export QT_QPA_PLATFORM=offscreen && \
		$(CTEST) --output-on-failure >/dev/null 2>&1 && \
		lcov --summary coverage.info 2>/dev/null || \
		echo "$(COLOR_YELLOW)Run 'make coverage' first$(COLOR_RESET)"

#=============================================================================
# Code Quality Targets
#=============================================================================

format: ## Format code using clang-format (if available)
	@echo "$(COLOR_BLUE)Formatting code...$(COLOR_RESET)"
	@command -v clang-format >/dev/null 2>&1 || { \
		echo "$(COLOR_YELLOW)clang-format not found. Install with: sudo apt install clang-format$(COLOR_RESET)"; \
		exit 0; \
	}
	@find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i -style=file 2>/dev/null || \
		find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i -style="{BasedOnStyle: Google, IndentWidth: 4}"
	@echo "$(COLOR_GREEN)✓ Code formatted$(COLOR_RESET)"

lint: ## Run static analysis (if available)
	@echo "$(COLOR_BLUE)Running static analysis...$(COLOR_RESET)"
	@command -v cppcheck >/dev/null 2>&1 || { \
		echo "$(COLOR_YELLOW)cppcheck not found. Install with: sudo apt install cppcheck$(COLOR_RESET)"; \
		exit 0; \
	}
	@cppcheck --enable=all --suppress=missingIncludeSystem \
		--suppress=unusedFunction --std=c++17 src/ 2>&1 | head -50
	@echo "$(COLOR_GREEN)✓ Static analysis complete$(COLOR_RESET)"

check: test lint ## Run tests and linting

#=============================================================================
# Clean Targets
#=============================================================================

clean: ## Clean build artifacts
	@echo "$(COLOR_BLUE)Cleaning build artifacts...$(COLOR_RESET)"
	@if [ -d $(BUILD_DIR_RELEASE) ]; then \
		cd $(BUILD_DIR_RELEASE) && $(NINJA) clean 2>/dev/null || true; \
	fi
	@if [ -d $(BUILD_DIR_DEBUG) ]; then \
		cd $(BUILD_DIR_DEBUG) && $(NINJA) clean 2>/dev/null || true; \
	fi
	@if [ -d $(BUILD_DIR_TEST) ]; then \
		cd $(BUILD_DIR_TEST) && $(NINJA) clean 2>/dev/null || true; \
	fi
	@if [ -d $(BUILD_DIR_COVERAGE) ]; then \
		cd $(BUILD_DIR_COVERAGE) && $(NINJA) clean 2>/dev/null || true; \
	fi
	@echo "$(COLOR_GREEN)✓ Clean complete$(COLOR_RESET)"

clean-all: ## Remove all build directories
	@echo "$(COLOR_BLUE)Removing all build directories...$(COLOR_RESET)"
	@rm -rf $(BUILD_DIR) $(BUILD_DIR_DEBUG) $(BUILD_DIR_RELEASE) \
		$(BUILD_DIR_COVERAGE) $(BUILD_DIR_TEST)
	@echo "$(COLOR_GREEN)✓ All build directories removed$(COLOR_RESET)"

distclean: clean-all ## Remove all generated files (build + cache)
	@echo "$(COLOR_BLUE)Removing all generated files...$(COLOR_RESET)"
	@rm -rf .cache .clangd .ccls-cache compile_commands.json
	@find . -name "*.o" -o -name "*.a" -o -name "*.so" | xargs rm -f 2>/dev/null || true
	@echo "$(COLOR_GREEN)✓ Distribution clean complete$(COLOR_RESET)"

#=============================================================================
# Run Targets
#=============================================================================

run: build ## Run the application (release)
	@echo "$(COLOR_BLUE)Running $(PROJECT_NAME)...$(COLOR_RESET)"
	@if [ -z "$$DISPLAY" ]; then \
		echo "$(COLOR_YELLOW)Warning: DISPLAY not set. Using offscreen mode.$(COLOR_RESET)"; \
		export QT_QPA_PLATFORM=offscreen; \
	fi
	@cd $(BUILD_DIR_RELEASE) && ./bin/$(PROJECT_NAME)

run-debug: build-debug ## Run the application (debug)
	@echo "$(COLOR_BLUE)Running $(PROJECT_NAME) (debug)...$(COLOR_RESET)"
	@if [ -z "$$DISPLAY" ]; then \
		echo "$(COLOR_YELLOW)Warning: DISPLAY not set. Using offscreen mode.$(COLOR_RESET)"; \
		export QT_QPA_PLATFORM=offscreen; \
	fi
	@cd $(BUILD_DIR_DEBUG) && ./bin/$(PROJECT_NAME)

run-headless: build ## Run application in headless mode
	@echo "$(COLOR_BLUE)Running $(PROJECT_NAME) (headless)...$(COLOR_RESET)"
	@cd $(BUILD_DIR_RELEASE) && \
		export QT_QPA_PLATFORM=offscreen && \
		./bin/$(PROJECT_NAME)

#=============================================================================
# Installation Targets
#=============================================================================

install: build ## Install the application
	@echo "$(COLOR_BLUE)Installing $(PROJECT_NAME)...$(COLOR_RESET)"
	@cd $(BUILD_DIR_RELEASE) && sudo $(NINJA) install
	@echo "$(COLOR_GREEN)✓ Installation complete$(COLOR_RESET)"

uninstall: ## Uninstall the application
	@echo "$(COLOR_BLUE)Uninstalling $(PROJECT_NAME)...$(COLOR_RESET)"
	@sudo rm -f /usr/local/bin/$(PROJECT_NAME)
	@sudo rm -rf /usr/local/share/$(PROJECT_NAME)
	@echo "$(COLOR_GREEN)✓ Uninstallation complete$(COLOR_RESET)"

#=============================================================================
# Development Workflow Targets
#=============================================================================

dev: build-debug ## Setup development environment
	@echo "$(COLOR_BLUE)Setting up development environment...$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)✓ Development environment ready$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)Useful commands:$(COLOR_RESET)"
	@echo "  make build-debug    - Build debug version"
	@echo "  make test           - Run tests"
	@echo "  make run-debug      - Run debug version"
	@echo "  make format         - Format code"
	@echo "  make lint           - Run static analysis"

watch: ## Watch for changes and rebuild (requires inotify-tools)
	@command -v inotifywait >/dev/null 2>&1 || { \
		echo "$(COLOR_YELLOW)inotifywait not found. Install with: sudo apt install inotify-tools$(COLOR_RESET)"; \
		exit 1; \
	}
	@echo "$(COLOR_BLUE)Watching for changes... (Ctrl+C to stop)$(COLOR_RESET)"
	@while true; do \
		inotifywait -r -e modify,create,delete src/ tests/ CMakeLists.txt 2>/dev/null && \
		echo "$(COLOR_BLUE)Changes detected, rebuilding...$(COLOR_RESET)" && \
		$(MAKE) build-debug; \
	done

quick-test: build-test ## Quick test cycle (build + test)
	@$(MAKE) build-test
	@$(MAKE) test-quick

#=============================================================================
# Documentation Targets
#=============================================================================

docs: ## Generate documentation (if Doxygen available)
	@echo "$(COLOR_BLUE)Generating documentation...$(COLOR_RESET)"
	@command -v doxygen >/dev/null 2>&1 || { \
		echo "$(COLOR_YELLOW)doxygen not found. Install with: sudo apt install doxygen graphviz$(COLOR_RESET)"; \
		exit 0; \
	}
	@if [ -f Doxyfile ]; then \
		doxygen Doxyfile; \
		echo "$(COLOR_GREEN)✓ Documentation generated: docs/html/index.html$(COLOR_RESET)"; \
	else \
		echo "$(COLOR_YELLOW)No Doxyfile found. Creating basic one...$(COLOR_RESET)"; \
		doxygen -g Doxyfile 2>/dev/null || true; \
	fi

#=============================================================================
# CI/CD Targets
#=============================================================================

ci-build: check-deps ## CI build target
	@mkdir -p $(BUILD_DIR_RELEASE)
	@cd $(BUILD_DIR_RELEASE) && \
		$(CMAKE) -G $(CMAKE_GENERATOR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_RELEASE) \
			.. && \
		$(NINJA) -j$(JOBS)

ci-test: build-test ## CI test target
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		$(CTEST) --output-on-failure -T Test

ci-coverage: build-coverage ## CI coverage target
	@cd $(BUILD_DIR_COVERAGE) && \
		export QT_QPA_PLATFORM=offscreen && \
		$(CTEST) --output-on-failure && \
		$(NINJA) coverage 2>/dev/null || \
		(lcov --capture --directory . --output-file coverage.info && \
		 lcov --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' --output-file coverage.info)

ci: ci-build ci-test ## Run full CI pipeline

#=============================================================================
# Utility Targets
#=============================================================================

info: ## Show build system information
	@echo "$(COLOR_BOLD)Build System Information$(COLOR_RESET)"
	@echo ""
	@echo "Project: $(PROJECT_NAME) v$(PROJECT_VERSION)"
	@echo "Build Type: $(BUILD_TYPE)"
	@echo "Jobs: $(JOBS)"
	@echo ""
	@echo "$(COLOR_BOLD)Tools:$(COLOR_RESET)"
	@echo "  CMake: $$($(CMAKE) --version 2>/dev/null | head -1 || echo 'Not found')"
	@echo "  Ninja: $$($(NINJA) --version 2>/dev/null || echo 'Not found')"
	@echo "  Qt6: $$(pkg-config --modversion Qt6Core 2>/dev/null || echo 'Not found')"
	@echo ""
	@echo "$(COLOR_BOLD)Build Directories:$(COLOR_RESET)"
	@ls -d $(BUILD_DIR)* 2>/dev/null | sed 's/^/  /' || echo "  (none)"
	@echo ""

version: ## Show version information
	@echo "$(PROJECT_NAME) v$(PROJECT_VERSION)"

#=============================================================================
# Advanced Targets
#=============================================================================

compile-commands: ## Generate compile_commands.json for IDE support
	@echo "$(COLOR_BLUE)Generating compile_commands.json...$(COLOR_RESET)"
	@mkdir -p $(BUILD_DIR_DEBUG)
	@cd $(BUILD_DIR_DEBUG) && \
		$(CMAKE) -G $(CMAKE_GENERATOR) \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE_DEBUG) \
			-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			.. >/dev/null 2>&1
	@ln -sf $(BUILD_DIR_DEBUG)/compile_commands.json . 2>/dev/null || true
	@echo "$(COLOR_GREEN)✓ compile_commands.json generated$(COLOR_RESET)"

package: build ## Create distribution package
	@echo "$(COLOR_BLUE)Creating distribution package...$(COLOR_RESET)"
	@mkdir -p dist
	@cd $(BUILD_DIR_RELEASE) && \
		$(NINJA) package || \
		(cmake --build . --target package)
	@echo "$(COLOR_GREEN)✓ Package created in dist/$(COLOR_RESET)"

benchmark: build-test ## Run performance benchmarks
	@echo "$(COLOR_BLUE)Running benchmarks...$(COLOR_RESET)"
	@cd $(BUILD_DIR_TEST) && \
		export QT_QPA_PLATFORM=offscreen && \
		./tests/HyperlocalWeatherTests --gtest_filter="*Benchmark*:*Performance*" \
			--gtest_color=yes
	@echo "$(COLOR_GREEN)✓ Benchmarks complete$(COLOR_RESET)"

#=============================================================================
# Special Targets
#=============================================================================

.SILENT: info version

# Print build information
print-%:
	@echo '$*=$($*)'

