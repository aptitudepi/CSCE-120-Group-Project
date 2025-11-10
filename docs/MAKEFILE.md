# Makefile Build System Documentation

## Overview

The Hyperlocal Weather project includes a comprehensive Makefile system that automates building, testing, code quality checks, and deployment workflows. This system provides a unified interface for all development tasks.

## Quick Start

```bash
# Show all available commands
make help

# Build the project
make build

# Run tests
make test

# Run the application
make run
```

## Features

### ✅ Build Management
- **Multiple build types**: Release, Debug, Test, Coverage
- **Parallel builds**: Automatically uses all CPU cores
- **Separate build directories**: No conflicts between build types
- **Incremental builds**: Only rebuilds what changed

### ✅ Testing
- **Comprehensive test suite**: Unit, integration, service tests
- **Test filtering**: Run specific test suites
- **Coverage reports**: HTML and terminal summaries
- **CI-friendly**: Optimized for automated testing

### ✅ Code Quality
- **Code formatting**: Automatic formatting with clang-format
- **Static analysis**: cppcheck integration
- **IDE support**: Generate compile_commands.json

### ✅ Development Tools
- **Watch mode**: Auto-rebuild on file changes
- **Quick test cycle**: Build and test in one command
- **Development setup**: One-command dev environment

### ✅ CI/CD Integration
- **CI-specific targets**: Optimized for automated environments
- **GitHub Actions**: Pre-configured workflow
- **Artifact generation**: Test results and coverage reports

## Command Reference

### Build Commands

| Command | Description |
|---------|-------------|
| `make build` | Build release version (default) |
| `make build-debug` | Build debug version with symbols |
| `make build-test` | Build with tests enabled |
| `make build-coverage` | Build with coverage instrumentation |
| `make rebuild` | Clean and rebuild from scratch |
| `make rebuild-debug` | Clean and rebuild debug version |

### Test Commands

| Command | Description |
|---------|-------------|
| `make test` | Run all tests |
| `make test-unit` | Run unit tests only |
| `make test-integration` | Run integration tests only |
| `make test-services` | Run service tests only |
| `make test-quick` | Run quick tests (no network) |
| `make test-verbose` | Run tests with verbose output |
| `make quick-test` | Quick test cycle (build + test) |

### Coverage Commands

| Command | Description |
|---------|-------------|
| `make coverage` | Generate full coverage report (HTML) |
| `make coverage-summary` | Show coverage summary in terminal |

### Code Quality Commands

| Command | Description |
|---------|-------------|
| `make format` | Format code using clang-format |
| `make lint` | Run static analysis (cppcheck) |
| `make check` | Run tests and linting |

### Clean Commands

| Command | Description |
|---------|-------------|
| `make clean` | Clean build artifacts |
| `make clean-all` | Remove all build directories |
| `make distclean` | Remove all generated files |

### Run Commands

| Command | Description |
|---------|-------------|
| `make run` | Run release version |
| `make run-debug` | Run debug version |
| `make run-headless` | Run in headless mode (no GUI) |

### Development Commands

| Command | Description |
|---------|-------------|
| `make dev` | Setup development environment |
| `make watch` | Watch for changes and auto-rebuild |
| `make compile-commands` | Generate compile_commands.json for IDE |

### Installation Commands

| Command | Description |
|---------|-------------|
| `make install` | Install the application |
| `make uninstall` | Uninstall the application |

### CI/CD Commands

| Command | Description |
|---------|-------------|
| `make ci-build` | CI build target |
| `make ci-test` | CI test target |
| `make ci-coverage` | CI coverage target |
| `make ci` | Run full CI pipeline |

## Usage Examples

### Daily Development Workflow

```bash
# Morning setup
make dev              # Setup development environment
make build-debug      # Build debug version

# During development
make watch            # Auto-rebuild on changes (in another terminal)
make test             # Run tests frequently
make format           # Format code before committing

# Before committing
make check            # Run tests + linting
make coverage         # Check coverage
```

### Testing Workflow

```bash
# Run all tests
make test

# Run specific test suites
make test-unit
make test-integration
make test-services

# Generate coverage report
make build-coverage
make coverage
# Open: build-coverage/coverage/index.html
```

### Release Workflow

```bash
# Build release
make build

# Run full test suite
make test

# Generate coverage
make coverage

# Package for distribution
make package
```

## Configuration

### Environment Variables

Customize the build system using environment variables:

```bash
# Set build type
BUILD_TYPE=Debug make build

# Set number of parallel jobs
JOBS=8 make build

# Combined example
BUILD_TYPE=Debug JOBS=8 make build
```

### Build Directories

The Makefile uses separate build directories:

- `build-release/` - Release builds
- `build-debug/` - Debug builds  
- `build-test/` - Test builds
- `build-coverage/` - Coverage builds
- `build-ci/` - CI builds

This allows multiple build configurations simultaneously.

## Advanced Features

### Code Formatting

The project includes a `.clang-format` file. Format code with:

```bash
make format
```

The formatter uses Google style with 4-space indentation and 100-character line limit.

### Static Analysis

Run static analysis with cppcheck:

```bash
make lint
```

### IDE Support

Generate `compile_commands.json` for IDE autocomplete:

```bash
make compile-commands
```

This creates a symlink in the project root that IDEs like VSCode, CLion, and Qt Creator can use.

### Watch Mode

Automatically rebuild when files change (requires `inotify-tools`):

```bash
make watch
```

Install inotify-tools:
```bash
sudo apt install inotify-tools
```

### Performance Benchmarks

Run performance benchmarks:

```bash
make benchmark
```

## CI/CD Integration

### GitHub Actions

The project includes a pre-configured GitHub Actions workflow (`.github/workflows/build.yml`) that uses the Makefile:

```yaml
- name: Build
  run: make ci-build

- name: Test
  run: make ci-test
```

### Custom CI

For other CI systems, use the `ci-*` targets:

```bash
make ci-build      # Build
make ci-test       # Test
make ci-coverage   # Coverage
```

## Troubleshooting

### Dependencies Not Found

```bash
# Check dependencies
make check-deps

# Install dependencies
make install-deps
```

### Build Fails

```bash
# Clean and rebuild
make clean-all
make build

# Check build information
make info
```

### Tests Fail

```bash
# Run tests with verbose output
make test-verbose

# Run quick tests (no network)
make test-quick
```

### Coverage Not Generated

```bash
# Install coverage tools
sudo apt install -y lcov gcovr

# Rebuild with coverage
make build-coverage
make coverage
```

## Best Practices

1. **Always use `make build` before committing**
2. **Run `make test` before pushing code**
3. **Use `make format` to maintain code style**
4. **Check `make coverage` regularly**
5. **Use `make clean-all` when switching branches**

## File Structure

```
.
├── Makefile              # Main Makefile
├── Makefile.ci          # CI-specific targets
├── .clang-format        # Code formatting configuration
├── .github/
│   └── workflows/
│       └── build.yml    # GitHub Actions workflow
└── docs/
    └── MAKEFILE.md      # This file
```

## Additional Resources

- [Build Documentation](BUILD.md) - Detailed build instructions
- [Architecture Documentation](ARCHITECTURE.md) - System design
- [Setup Guide](SETUP.md) - Environment setup
- [Makefile README](../Makefile.README.md) - Quick reference

