# CI/CD Pipeline Guide

## Overview

The Hyperlocal Weather application uses GitHub Actions for continuous integration and continuous deployment (CI/CD). This automated system ensures code quality, runs tests, and creates releases automatically.

## Quick Start

### For Developers

Every time you push code:
1. GitHub Actions automatically builds your code
2. All tests are executed
3. Code quality checks are performed
4. Results are reported in the PR or commit

**No manual intervention needed!**

### For Releases

To create a new release:

```bash
# Tag your release
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

The system will automatically:
- Run all tests
- Build the release package
- Create a GitHub release
- Upload release artifacts

## Workflow Files

The CI/CD system consists of three main workflows:

### 1. CI/CD Pipeline (`ci.yml`)

**Purpose**: Comprehensive testing and quality checks

**When it runs**:
- Every push to main/develop branches
- Every pull request

**What it does**:
- Builds in Release and Debug modes
- Runs all unit and integration tests
- Generates code coverage reports
- Performs static analysis (cppcheck)
- Checks code formatting

**Results**:
- Build artifacts
- Test results
- Coverage reports
- Lint results

### 2. Build and Test (`build-test.yml`)

**Purpose**: Fast feedback for developers

**When it runs**:
- Every push to any branch
- Every pull request
- Manually triggered

**What it does**:
- Quick build verification
- Unit test execution
- Integration test execution

**Results**:
- Pass/fail status
- Test logs

### 3. Release (`release.yml`)

**Purpose**: Automated release creation

**When it runs**:
- When version tags are pushed (v1.0.0, v2.1.0, etc.)
- Manually triggered

**What it does**:
- Runs full test suite
- Builds optimized release binary
- Creates tarball package
- Publishes GitHub release
- Deploys documentation

**Results**:
- Downloadable release package
- GitHub release page
- Published documentation

## Status Badges

The README displays three status badges:

```markdown
[![CI/CD Pipeline](https://github.com/aptitudepi/CSCE-120-Group-Project/workflows/CI%2FCD%20Pipeline/badge.svg)]
[![Build and Test](https://github.com/aptitudepi/CSCE-120-Group-Project/workflows/Build%20and%20Test/badge.svg)]
[![Release](https://github.com/aptitudepi/CSCE-120-Group-Project/workflows/Release/badge.svg)]
```

These badges show the current status of each workflow.

## Viewing Build Results

1. Go to the repository on GitHub
2. Click the "Actions" tab
3. Select a workflow run to see details
4. Click on individual jobs to see logs

## Downloading Artifacts

Build artifacts are available for 90 days:

1. Navigate to Actions → Workflow run
2. Scroll to the "Artifacts" section
3. Click to download:
   - `hyperlocal-weather-{sha}`: Release binary
   - `test-results-{type}`: Test output
   - `coverage-report`: Coverage data
   - `lint-results`: Static analysis results

## Local Testing Before Push

Always test locally before pushing:

```bash
# Build and test
make build-test
make test

# Check code style
make lint

# Generate coverage
make coverage
```

This matches what CI/CD will run.

## Workflow Triggers

### Automatic Triggers

| Event | Workflow | Branch |
|-------|----------|--------|
| Push | CI/CD, Build/Test | main, master, develop, copilot/** |
| Pull Request | CI/CD, Build/Test | main, master, develop |
| Tag Push (v*) | Release | any |

### Manual Triggers

All workflows can be manually triggered:

1. Go to Actions tab
2. Select workflow
3. Click "Run workflow"
4. Choose branch/parameters
5. Click "Run workflow"

## Release Process

### Semantic Versioning

Follow semantic versioning for tags:
- `v1.0.0` - Major release
- `v1.1.0` - Minor release (new features)
- `v1.1.1` - Patch release (bug fixes)

### Creating a Release

**Method 1: Via Git Tag**

```bash
# Create annotated tag
git tag -a v1.2.0 -m "Release version 1.2.0 - Added feature X"

# Push tag
git push origin v1.2.0
```

**Method 2: Via GitHub UI**

1. Go to Releases → Draft a new release
2. Create new tag (e.g., v1.2.0)
3. Add release title and description
4. Click "Publish release"

**Method 3: Via Workflow Dispatch**

1. Go to Actions → Release workflow
2. Click "Run workflow"
3. Enter version (e.g., v1.2.0)
4. Click "Run workflow"

### What Gets Released

The release package includes:
- Compiled `HyperlocalWeather` binary
- README.md
- QUICKSTART.md
- Documentation files

Package name: `hyperlocal-weather-{version}-linux-x86_64.tar.gz`

## Understanding Test Results

### Test Types

1. **Unit Tests**: Test individual components
   - Fast execution
   - No external dependencies
   - Run on every push

2. **Integration Tests**: Test component interactions
   - May use network
   - Test service integration
   - Run on every push

3. **End-to-End Tests**: Test complete workflows
   - Full application flow
   - Comprehensive validation
   - Run before releases

### Test Output

Tests run with Google Test framework:
- Green = Passed
- Red = Failed
- Yellow = Skipped

View detailed results in:
- Workflow logs
- Downloaded test artifacts
- CTest XML reports

## Code Coverage

Coverage reports show which code is tested:

**Viewing Coverage:**
1. Download `coverage-report` artifact
2. Open `coverage/index.html` in browser
3. Browse coverage by file/function

**Coverage Goals:**
- Overall: > 80%
- Critical modules: > 90%
- Services: > 85%

## Troubleshooting

### "Tests Failed" Error

1. Check the test logs in Actions tab
2. Identify failing tests
3. Run tests locally:
   ```bash
   make test-verbose
   ```
4. Fix the failing tests
5. Push the fix

### "Build Failed" Error

1. Check build logs
2. Look for compilation errors
3. Build locally:
   ```bash
   make build-debug
   ```
4. Fix compilation issues
5. Push the fix

### "Coverage Too Low" Warning

1. Download coverage report
2. Identify untested code
3. Add tests for uncovered areas
4. Push updated tests

### Workflow Stuck or Slow

- Check GitHub Actions status page
- Verify no dependency issues
- Clear cache and retry:
  - Go to Actions → Caches
  - Delete cache entries
  - Re-run workflow

## Performance

### Build Times

Typical build times:
- Quick build: ~3-5 minutes
- Full CI/CD: ~8-12 minutes
- Release: ~10-15 minutes

### Optimization

The workflows use several optimizations:
- **Caching**: Build artifacts and dependencies
- **Matrix strategy**: Parallel builds
- **Conditional jobs**: Skip unnecessary work
- **Artifact pruning**: Keep only relevant files

## Best Practices

### Before Pushing

✅ Run tests locally
✅ Check code formatting
✅ Update documentation
✅ Write meaningful commit messages

### During Development

✅ Push frequently to get fast feedback
✅ Review CI results promptly
✅ Fix failing tests immediately
✅ Monitor coverage metrics

### For Releases

✅ Update CHANGELOG.md
✅ Bump version numbers
✅ Test thoroughly before tagging
✅ Write clear release notes
✅ Verify release package

## Integration with Development Workflow

### Pull Request Workflow

1. Create feature branch
2. Make changes and commit
3. Push branch
4. Create pull request
5. CI/CD runs automatically
6. Review results
7. Fix any issues
8. Merge when all checks pass

### Main Branch Workflow

1. Merge PR to main
2. CI/CD runs full suite
3. All tests must pass
4. Code ready for release
5. Tag for release when ready

## Advanced Usage

### Custom Build Configurations

Edit `.github/workflows/ci.yml` to add configurations:

```yaml
matrix:
  build_type: [Release, Debug, RelWithDebInfo]
  compiler: [gcc, clang]
```

### Additional Test Suites

Add new test jobs:

```yaml
performance-tests:
  runs-on: ubuntu-24.04
  steps:
    - name: Run benchmarks
      run: make benchmark
```

### Deployment Stages

Add staging deployment:

```yaml
deploy-staging:
  needs: test
  if: github.ref == 'refs/heads/develop'
  steps:
    - name: Deploy to staging
      run: ./deploy-staging.sh
```

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Workflow Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)
- [Qt6 in CI/CD](https://doc.qt.io/qt-6/continuous-integration.html)
- [Google Test Guide](https://google.github.io/googletest/)

## Support

For CI/CD issues:
1. Check workflow logs
2. Review this documentation
3. Consult `.github/workflows/README.md`
4. Open an issue with:
   - Workflow run URL
   - Error messages
   - Expected vs actual behavior

## Summary

The CI/CD system provides:
- ✅ Automated testing on every push
- ✅ Code quality enforcement
- ✅ Fast feedback for developers
- ✅ Automatic release creation
- ✅ Comprehensive reporting

**Result**: Higher code quality, fewer bugs, faster delivery!
