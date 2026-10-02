// DebugCapture.hpp - headless screenshot helper.
//
// Reads the current back buffer and writes it as a 24-bit BMP. Used by the
// --shot CLI option (and handy for CI-style smoke tests); deliberately BMP
// so the debug path stays dependency-free.
#pragma once

#include <string>

namespace maze::debug {

// Captures the frame currently in the back buffer (call before swap).
bool saveScreenshotBMP(const std::string& path, int width, int height);

} // namespace maze::debug
