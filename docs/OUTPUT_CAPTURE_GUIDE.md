# Sample Output Capture Guide

## Overview

This guide explains how to capture sample outputs for Artifact 3 submission (≤5 pages). The outputs should demonstrate the application's functionality, correctness, and user experience.

## Recommended Output Pages

### Page 1: Application Launch and Main Interface
**Purpose:** Show the complete application interface and basic functionality

**Steps to Capture:**
1. Launch the application: `./build/bin/HyperlocalWeather`
2. Take a screenshot showing:
   - Main window with title bar
   - Location input area
   - Service selector (showing "Aggregated")
   - Current weather display (if location already entered)
   - Forecast list area

**What to Highlight:**
- Clean, professional UI
- Service selection dropdown
- All major UI components visible

**Annotation:** Add text labels if needed to identify key features

---

### Page 2: Forecast Display with Aggregated Data
**Purpose:** Demonstrate multi-source aggregation working

**Steps to Capture:**
1. Enter a location (e.g., Austin, TX: 30.2672, -97.7431)
2. Select "Aggregated" service
3. Wait for forecast to load (5-10 seconds)
4. Scroll to show multiple forecast periods
5. Take screenshot showing:
   - Current weather with temperature, condition
   - Forecast list with 5-7 forecast cards
   - Precipitation probabilities and intensities visible
   - Wind information displayed

**What to Highlight:**
- Forecast cards showing different time periods
- Precipitation predictions (probability and intensity)
- Temperature variations over time
- Service indicator showing "Aggregated"

**Annotation:** 
- Add arrow pointing to aggregated service indicator
- Highlight a forecast card showing precipitation prediction

---

### Page 3: Settings and Configuration
**Purpose:** Show moving average and aggregation configuration options

**Steps to Capture:**
1. Click "Settings" button
2. Take screenshot of Settings page showing:
   - Aggregation options
   - Moving average settings (if implemented in UI)
   - Data retention settings
   - API key configuration (if visible)

**Alternative:** If Settings UI not fully implemented, show:
- Service selector with all options visible
- Console output showing aggregation configuration
- Or show code snippet from WeatherController showing configuration

**What to Highlight:**
- Configuration options available to users
- Advanced features (aggregation, smoothing)

**Annotation:**
- Label key configuration options
- Show how users can customize behavior

---

### Page 4: Error Handling and Robustness
**Purpose:** Demonstrate graceful error handling

**Steps to Capture:**

**Option A: Network Error**
1. Disconnect internet (or block network access)
2. Try to fetch forecast
3. Capture error dialog showing clear error message
4. Show that application doesn't crash

**Option B: Invalid Input**
1. Enter invalid coordinates (e.g., 999, 999)
2. Capture error message
3. Show application remains responsive

**Option C: Service Fallback**
1. Show console output with service failure
2. Show that aggregator falls back to available service
3. Or show cached data being used

**What to Highlight:**
- Clear error messages
- Application stability (doesn't crash)
- Graceful degradation

**Annotation:**
- Explain the error scenario
- Show recovery mechanism

---

### Page 5: Testing and Validation
**Purpose:** Show test execution and coverage

**Steps to Capture:**

**Option A: Test Execution Output**
```bash
cd build
make test
# or
ctest -V
```
Capture terminal output showing:
- All tests passing
- Test names and results
- Coverage summary (if available)

**Option B: Test Code and Results**
- Screenshot of test file (e.g., test_WeatherAggregator.cpp)
- Terminal showing test execution
- Test results summary

**Option C: Integration Test Output**
- Show end-to-end test passing
- Demonstrate complete flow working
- Show validation of weighted average calculations

**What to Highlight:**
- Comprehensive test coverage
- All critical tests passing
- Test organization (unit, integration)

**Annotation:**
- Identify test categories
- Show key test results

---

## Alternative Page Suggestions

### Alternative Page 3: Historical Data Storage
- Screenshot of database contents
- Or show HistoricalDataManager storing/retrieving data
- Demonstrate time-series data capability

### Alternative Page 4: Moving Average Smoothing
- Before/after comparison of forecasts
- Show smoothed vs. unsmoothed data
- Graph or table showing smoothing effect

### Alternative Page 5: Performance Metrics
- PerformanceMonitor output
- Response time measurements
- Cache hit rate statistics

---

## Screenshot Tools

### Linux/WSL
```bash
# Using gnome-screenshot (install: sudo apt install gnome-screenshot)
gnome-screenshot -a  # Interactive area selection
gnome-screenshot -w  # Current window
gnome-screenshot -f screenshot.png  # Full screen

# Using ImageMagick (install: sudo apt install imagemagick)
import -window root output.png  # Full screen
import output.png  # Interactive selection

# Using scrot (install: sudo apt install scrot)
scrot -s screenshot.png  # Interactive selection
```

### Windows
- **Snipping Tool:** Built-in, press Windows + Shift + S
- **Print Screen:** Capture full screen
- **Greenshot:** Free screenshot tool with annotations

### macOS
- **Cmd + Shift + 3:** Full screen
- **Cmd + Shift + 4:** Area selection
- **Cmd + Shift + 4 + Space:** Window capture

---

## Terminal Output Capture

### Capture Command Output
```bash
# Run command and save to file
make test | tee test_output.txt

# Capture with timestamps
script -c "make test" test_output.log

# Capture specific terminal session
ttyrec output.ttyrec
# Later: ttyplay output.ttyrec to replay
```

### Formatting Terminal Output
1. Use clear fonts (Monaco, Consolas, Ubuntu Mono)
2. Increase font size for readability
3. Use color output (if supported in document)
4. Remove unnecessary clutter (scrollback)

---

## Image Editing and Annotation

### Recommended Tools
- **Linux:** GIMP, Krita, KolourPaint
- **Windows:** Paint.NET, GIMP
- **macOS:** Preview (built-in), GIMP

### Annotation Tips
1. **Add Labels:**
   - Use arrows to point to features
   - Add text boxes explaining functionality
   - Use different colors for different types of annotations

2. **Highlighting:**
   - Box important areas
   - Use semi-transparent overlays
   - Circle key UI elements

3. **Consistency:**
   - Use same annotation style across all images
   - Keep text size readable
   - Use contrasting colors

---

## Combining Multiple Screenshots

### Method 1: Single Page Layout
- Arrange 2-3 smaller screenshots on one page
- Add captions below each
- Use consistent spacing

### Method 2: Side-by-Side Comparison
- Show before/after states
- Configuration vs. result
- Original vs. aggregated forecast

### Method 3: Grid Layout
- 2x2 or 2x3 grid
- Each cell shows different feature
- Number or label each cell

---

## File Format Recommendations

1. **High Resolution:** Capture at native resolution (1200x800 or higher)
2. **Format:** PNG (lossless) or PDF (for documents)
3. **Compression:** Use lossless compression for clarity
4. **File Size:** Keep individual images <2MB for easy sharing

---

## Creating the Final Document

### Option 1: PDF with Multiple Pages
```bash
# Combine screenshots into PDF using ImageMagick
convert screenshot1.png screenshot2.png ... output.pdf

# Or use LibreOffice Writer
# Insert images, add annotations, export as PDF
```

### Option 2: Markdown with Embedded Images
```markdown
# Sample Outputs

## Page 1: Main Interface
![Main Interface](images/page1_main_interface.png)
*Caption: Application showing location input and service selector*

## Page 2: Forecast Display
![Forecasts](images/page2_forecasts.png)
*Caption: Aggregated forecasts showing precipitation predictions*
```

Convert to PDF: Use `pandoc` or online markdown-to-PDF converters

### Option 3: PowerPoint/Google Slides
- Create presentation with screenshots
- Add annotations
- Export as PDF

---

## Quality Checklist

Before submitting, ensure:
- [ ] All 5 pages captured
- [ ] Images are clear and readable
- [ ] Key features visible in screenshots
- [ ] Annotations helpful (if added)
- [ ] Consistent style across pages
- [ ] Text in images is readable (zoom in if needed)
- [ ] File size reasonable (<10MB total)
- [ ] Format is PDF or easy-to-view format

---

## Example Workflow

1. **Prepare Application:**
   ```bash
   make build
   ./build/bin/HyperlocalWeather
   ```

2. **Capture Page 1:** Main interface screenshot
3. **Capture Page 2:** Enter location, show forecasts
4. **Capture Page 3:** Open settings or show configuration
5. **Capture Page 4:** Demonstrate error handling
6. **Capture Page 5:** Run tests, capture output

7. **Edit Images:** Add annotations, labels, arrows
8. **Combine:** Create PDF with all pages
9. **Review:** Ensure everything is clear and demonstrates features

---

## Troubleshooting

**Application won't launch:**
- Check build succeeded: `make build`
- Check dependencies installed: `make install-deps`
- Run from correct directory: `./build/bin/HyperlocalWeather`

**Screenshots too small:**
- Increase application window size
- Use higher resolution capture
- Zoom in on specific areas for detail shots

**Terminal output unclear:**
- Increase terminal font size
- Use full-width terminal
- Clean output (remove previous commands)

**Can't capture error scenarios:**
- Simulate errors using invalid inputs
- Show error handling code instead
- Use test cases that trigger errors

---

Good luck capturing your sample outputs!

