# CI/CD Documentation

## Overview

The Hyperlocal Weather project includes a comprehensive GitHub Actions CI/CD pipeline that automatically builds, tests, and creates releases on every push.

## Workflows

### 1. CI/CD Pipeline (`ci.yml`)

**Triggers:**
- Push to `main`, `develop`, or `release/**` branches
- Push of tags matching `v*.*.*`
- Pull requests to `main` or `develop`

**Jobs:**
- **build-and-test**: Builds and tests on Release and Debug configurations
- **coverage**: Generates code coverage reports
- **code-quality**: Runs code formatting and static analysis
- **release**: Creates GitHub releases when tests pass
- **build-status**: Final status check

**Features:**
- Matrix builds (Release + Debug)
- Test result artifacts
- Coverage reports
- Static analysis results
- Automatic release creation

### 2. Build and Test (`build.yml`)

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`

**Jobs:**
- **build**: Builds and runs tests
- **coverage**: Generates code coverage

**Use Case:** Quick feedback on code changes

### 3. Release (`release.yml`)

**Triggers:**
- Push of tags matching `v*.*.*`
- Manual workflow dispatch

**Jobs:**
- **build-and-test**: Ensures all tests pass before release
- **create-release**: Creates GitHub release with binaries

**Features:**
- Automatic version detection
- Release archive creation (tar.gz and zip)
- Release notes generation
- Binary distribution

### 4. PR Checks (`pr-checks.yml`)

**Triggers:**
- Pull request opened, updated, or reopened

**Jobs:**
- **quick-build**: Fast build verification
- **format-check**: Code formatting verification
- **lint-check**: Static analysis
- **test-suite**: Full test suite
- **pr-status**: Overall PR status

**Use Case:** Quick feedback on pull requests

## Usage

### Automatic Workflows

Workflows run automatically based on their triggers. No manual intervention needed.

### Creating a Release

#### Option 1: Tag-based Release (Recommended)

```bash
# Create and push a tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0
```

The `release.yml` workflow will automatically:
1. Build the project
2. Run all tests
3. Create a GitHub release if tests pass
4. Upload release binaries

#### Option 2: Manual Release

1. Go to Actions → Release workflow
2. Click "Run workflow"
3. Enter version number (e.g., `1.0.0`)
4. Click "Run workflow"

### Viewing Results

- **Actions Tab**: View all workflow runs and their status
- **Artifacts**: Download build artifacts and test results
- **Releases**: View created releases and download binaries

## Workflow Status Badges

The README includes status badges that show the current status of workflows:

```markdown
[![CI/CD Pipeline](https://github.com/OWNER/REPO/workflows/CI%2FCD%20Pipeline/badge.svg)]
[![Build and Test](https://github.com/OWNER/REPO/workflows/Build%20and%20Test/badge.svg)]
[![Release](https://github.com/OWNER/REPO/workflows/Release/badge.svg)]
```

## Configuration

### Environment Variables

Workflows use these environment variables:

- `QT_VERSION`: Qt version to use (default: "6.4")
- `BUILD_TYPE`: Build type (default: "Release")

### Secrets

No secrets required for basic workflows. The `GITHUB_TOKEN` is automatically provided.

For Codecov integration, add `CODECOV_TOKEN` to repository secrets (optional).

## Release Process

### Automatic Release Creation

When you push a tag matching `v*.*.*`:

1. **Build and Test**: All tests must pass
2. **Version Detection**: Version extracted from tag
3. **Binary Creation**: Release binaries built
4. **Archive Creation**: tar.gz and zip archives created
5. **Release Notes**: Generated from commits since last tag
6. **GitHub Release**: Created with binaries attached

### Release Artifacts

Each release includes:
- `HyperlocalWeather-{version}-linux.tar.gz`
- `HyperlocalWeather-{version}-linux.zip`
- Release notes with changelog
- Binary executable

### Release Notes

Release notes are automatically generated:
- For tags: Commits since last tag
- For manual releases: Custom notes or default template

## Testing

### Test Execution

Tests run automatically on:
- Every push to main/develop
- Every pull request
- Before release creation

### Test Results

Test results are available as:
- Artifacts in GitHub Actions
- XML reports for CI integration
- Terminal output in workflow logs

### Coverage Reports

Coverage reports are generated:
- On pull requests
- On pushes to main branch
- Available as HTML artifacts

## Code Quality

### Formatting Check

Code formatting is checked using clang-format:
- Runs on every pull request
- Fails if code is not formatted
- Use `make format` to fix formatting

### Static Analysis

Static analysis runs using cppcheck:
- Runs on every pull request
- Non-blocking (warnings only)
- Results available as artifacts

## Troubleshooting

### Workflow Fails

1. Check the Actions tab for error details
2. Review test results in artifacts
3. Check build logs for compilation errors
4. Verify all dependencies are installed

### Release Not Created

1. Ensure all tests pass
2. Check that tag format matches `v*.*.*`
3. Verify workflow has `contents: write` permission
4. Check release workflow logs

### Coverage Not Generated

1. Ensure coverage tools are installed (lcov, gcovr)
2. Check that `ENABLE_COVERAGE=ON` is set
3. Review coverage job logs
4. Verify tests ran successfully

### Tests Fail

1. Check test output in workflow logs
2. Review test artifacts
3. Run tests locally: `make test`
4. Check for environment-specific issues

## Best Practices

1. **Always run tests before pushing**
   ```bash
   make test
   ```

2. **Use semantic versioning for tags**
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   ```

3. **Review PR checks before merging**
   - All checks must pass
   - Review code quality results
   - Check coverage reports

4. **Keep workflows up to date**
   - Update dependencies regularly
   - Review workflow logs for issues
   - Update workflow files as needed

5. **Monitor workflow performance**
   - Check workflow run times
   - Optimize slow jobs
   - Use caching where appropriate

## Integration with Makefile

The CI/CD workflows use the Makefile system:

```yaml
- name: Build
  run: make ci-build

- name: Test
  run: make ci-test

- name: Coverage
  run: make ci-coverage
```

This ensures consistency between local and CI builds.

## Additional Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Workflow README](../.github/workflows/README.md)
- [Makefile Documentation](MAKEFILE.md)
- [Build Documentation](BUILD.md)

