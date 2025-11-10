# Makefile Build System

This project includes a comprehensive Makefile system for automated building, testing, and development workflows.

## Quick Reference

### Essential Commands

```bash
make help          # Show all available targets
make build         # Build release version
make build-debug   # Build debug version
make test          # Run all tests
make run           # Run the application
make clean         # Clean build artifacts
```

### Build Targets

| Target | Description |
|-------|-------------|
| `build` | Build release version (default) |
| `build-debug` | Build debug version with symbols |
| `build-test` | Build with tests enabled |
| `build-coverage` | Build with coverage instrumentation |
| `rebuild` | Clean and rebuild from scratch |
| `rebuild-debug` | Clean and rebuild debug version |

### Test Targets

| Target | Description |
|-------|-------------|
| `test` | Run all tests |
| `test-unit` | Run unit tests only |
| `test-integration` | Run integration tests only |
| `test-services` | Run service tests only |
| `test-quick` | Run quick tests (no network) |
| `test-verbose` | Run tests with verbose output |
| `quick-test` | Quick test cycle (build + test) |

### Coverage Targets

| Target | Description |
|-------|-------------|
| `coverage` | Generate full coverage report (HTML) |
| `coverage-summary` | Show coverage summary in terminal |

### Code Quality Targets

| Target | Description |
|-------|-------------|
| `format` | Format code using clang-format |
| `lint` | Run static analysis (cppcheck) |
| `check` | Run tests and linting |

### Clean Targets

| Target | Description |
|-------|-------------|
| `clean` | Clean build artifacts |
| `clean-all` | Remove all build directories |
| `distclean` | Remove all generated files |

### Run Targets

| Target | Description |
|-------|-------------|
| `run` | Run release version |
| `run-debug` | Run debug version |
| `run-headless` | Run in headless mode (no GUI) |

### Development Targets

| Target | Description |
|-------|-------------|
| `dev` | Setup development environment |
| `watch` | Watch for changes and auto-rebuild |
| `compile-commands` | Generate compile_commands.json for IDE |

### Installation Targets

| Target | Description |
|-------|-------------|
| `install` | Install the application |
| `uninstall` | Uninstall the application |

### CI/CD Targets

| Target | Description |
|-------|-------------|
| `ci-build` | CI build target |
| `ci-test` | CI test target |
| `ci-coverage` | CI coverage target |
| `ci` | Run full CI pipeline |

## Usage Examples

### Development Workflow

```bash
# Initial setup
make install-deps    # Install dependencies
make dev             # Setup development environment

# Daily development
make build-debug      # Build debug version
make test             # Run tests
make run-debug        # Run and test

# Code quality
make format           # Format code
make lint             # Check code quality
make check            # Run tests + linting
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

# Run tests
make test

# Generate coverage
make coverage

# Package for distribution
make package
```

### Continuous Integration

The Makefile includes CI-specific targets that are optimized for automated environments:

```bash
make ci-build      # Build for CI
make ci-test       # Test for CI
make ci-coverage   # Coverage for CI
make ci            # Full CI pipeline
```

## Configuration

### Environment Variables

You can customize the build system using environment variables:

```bash
# Set build type
BUILD_TYPE=Debug make build

# Set number of parallel jobs
JOBS=8 make build

# Example: Build debug with 8 jobs
BUILD_TYPE=Debug JOBS=8 make build
```

### Build Directories

The Makefile uses separate build directories for different build types:

- `build-release/` - Release builds
- `build-debug/` - Debug builds
- `build-test/` - Test builds
- `build-coverage/` - Coverage builds
- `build-ci/` - CI builds

This allows you to have multiple build configurations simultaneously.

## Advanced Features

### Code Formatting

The project includes a `.clang-format` file. Format code with:

```bash
make format
```

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

### Performance Benchmarks

Run performance benchmarks:

```bash
make benchmark
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

## Integration with CI/CD

The Makefile is designed to work seamlessly with CI/CD systems. See `.github/workflows/build.yml` for an example GitHub Actions workflow.

### GitHub Actions

The project includes a GitHub Actions workflow that uses the Makefile:

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

## Best Practices

1. **Always use `make build` before committing
2. **Run `make test` before pushing code**
3. **Use `make format` to maintain code style**
4. **Check `make coverage` regularly**
5. **Use `make clean-all` when switching branches**

## Additional Resources

- [Build Documentation](docs/BUILD.md) - Detailed build instructions
- [Architecture Documentation](docs/ARCHITECTURE.md) - System design
- [Setup Guide](docs/SETUP.md) - Environment setup

