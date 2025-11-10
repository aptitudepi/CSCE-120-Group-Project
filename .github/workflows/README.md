# GitHub Actions CI/CD Workflows

This directory contains GitHub Actions workflows for automated building, testing, and releasing of the Hyperlocal Weather Forecasting Application.

## Workflows

### 1. CI/CD Pipeline (`ci.yml`)

**Triggers:**
- Push to `main`, `master`, `develop`, or `copilot/**` branches
- Pull requests to `main`, `master`, or `develop` branches

**Jobs:**
- **build-and-test**: Builds the application in both Release and Debug modes using a matrix strategy
  - Installs Qt6 and all dependencies
  - Caches build artifacts for faster subsequent builds
  - Runs full test suite with CTest
  - Uploads test results and build artifacts
  
- **coverage**: Generates code coverage reports
  - Builds with coverage instrumentation
  - Runs tests and generates coverage data
  - Uploads coverage to Codecov
  - Creates coverage artifacts
  
- **lint**: Performs static analysis
  - Runs cppcheck for code quality
  - Checks code formatting with clang-format
  - Uploads lint results

**Artifacts:**
- Test results (XML format)
- Build binaries (Release build)
- Coverage reports
- Lint results

### 2. Build and Test (`build-test.yml`)

**Triggers:**
- Push to `main`, `master`, `develop`, or `copilot/**` branches
- Pull requests to `main`, `master`, or `develop` branches
- Manual workflow dispatch

**Jobs:**
- **quick-build**: Fast build verification
  - Checks that the application compiles successfully
  - Verifies executable is created
  
- **unit-tests**: Runs unit tests
  - Builds with test support
  - Executes unit test suite
  - Runs CTest for comprehensive testing
  
- **integration-tests**: Runs integration tests
  - Tests end-to-end functionality
  - Validates service integration
  
- **success**: Reports overall status
  - Only runs if all previous jobs succeed

### 3. Release (`release.yml`)

**Triggers:**
- Push of version tags (e.g., `v1.0.0`, `v1.2.3`)
- Manual workflow dispatch with version input

**Jobs:**
- **test**: Pre-release testing
  - Runs full test suite before creating release
  - Ensures code quality before release
  
- **build-release**: Creates release package
  - Builds optimized Release version
  - Packages application with documentation
  - Creates compressed tarball
  - Creates GitHub release with artifacts
  - Adds release notes
  
- **deploy-docs**: Deploys documentation
  - Publishes documentation to GitHub Pages
  - Only runs for tagged releases

**Release Package Contents:**
- `HyperlocalWeather` binary
- `README.md`
- `QUICKSTART.md`
- Setup documentation

## Using the Workflows

### Automatic Triggers

The workflows automatically run on:
1. Every push to main branches
2. Every pull request
3. Every tag push (for releases)

### Manual Release

To create a release manually:

1. Go to Actions → Release workflow
2. Click "Run workflow"
3. Enter the version (e.g., `v1.0.0`)
4. Click "Run workflow"

### Creating a Tagged Release

To create a release with a tag:

```bash
# Create and push a version tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

This will automatically:
1. Run all tests
2. Build the release package
3. Create a GitHub release
4. Upload release artifacts
5. Deploy documentation

## Viewing Results

### Build Status

Check the status of workflows:
- Visit the [Actions tab](../../actions)
- Click on any workflow run to see details
- View logs for each job

### Badges

The README displays status badges for:
- CI/CD Pipeline status
- Build and Test status
- Release status

### Artifacts

Download build artifacts:
1. Go to Actions → Select workflow run
2. Scroll to "Artifacts" section
3. Download desired artifacts

## Workflow Caching

The workflows use GitHub Actions cache to speed up builds:
- **Build artifacts**: Cached by build type and source hash
- **Dependencies**: Qt6 libraries and tools

Cache is automatically invalidated when:
- CMakeLists.txt changes
- Source files (*.cpp, *.h) change

## Environment Variables

The workflows use these environment variables:
- `QT_QPA_PLATFORM=offscreen`: Runs Qt in headless mode for testing
- `GITHUB_TOKEN`: Automatically provided for releases

## Customization

### Modifying Build Matrix

To add more build configurations, edit `ci.yml`:

```yaml
matrix:
  build_type: [Release, Debug, RelWithDebInfo]
```

### Changing Test Filters

To run specific tests, modify the gtest filter:

```bash
./tests/HyperlocalWeatherTests --gtest_filter="YourTest*"
```

### Adding New Jobs

Add new jobs by editing the workflow files:

```yaml
new-job:
  name: My New Job
  runs-on: ubuntu-24.04
  steps:
    - uses: actions/checkout@v4
    # Add your steps here
```

## Troubleshooting

### Build Failures

1. Check the workflow logs in the Actions tab
2. Look for error messages in the build step
3. Verify dependencies are correctly installed

### Test Failures

1. Review test output in the workflow logs
2. Download test artifacts for detailed results
3. Run tests locally to reproduce issues:
   ```bash
   make build-test
   make test
   ```

### Cache Issues

To clear the cache:
1. Go to Actions → Caches
2. Delete relevant cache entries
3. Re-run the workflow

### Qt6 Issues

If Qt6 is not found:
- Verify the Ubuntu version is 24.04
- Check that all Qt6 packages are in the install step
- Ensure qml6-module packages are installed

## Best Practices

1. **Always run tests locally** before pushing
2. **Use meaningful commit messages** for clear workflow logs
3. **Tag releases properly** following semantic versioning (v1.2.3)
4. **Review test results** before merging pull requests
5. **Monitor workflow runs** for any failures or warnings

## Support

For issues with the CI/CD system:
1. Check the workflow logs
2. Review this documentation
3. Open an issue in the repository
4. Consult GitHub Actions documentation

## References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Creating CI/CD Pipelines with GitHub Actions](https://brandonkindred.medium.com/creating-your-first-ci-cd-pipeline-using-github-actions-81c668008582)
- [Qt6 Documentation](https://doc.qt.io/qt-6/)
- [CMake Documentation](https://cmake.org/documentation/)
