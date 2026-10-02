#include "DebugCapture.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

#include <glad/gl.h>

namespace maze::debug {

bool saveScreenshotBMP(const std::string& path, int width, int height) {
    if (width <= 0 || height <= 0) return false;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

    // BMP rows are bottom-up, BGR, and padded to 4-byte boundaries.
    // glReadPixels also returns rows bottom-up (row 0 = framebuffer bottom),
    // so file row y maps to buffer row y directly - no extra flip.
    const int rowBytes = width * 3;
    const int paddedRow = (rowBytes + 3) & ~3;
    const std::uint32_t dataSize =
        static_cast<std::uint32_t>(paddedRow) * static_cast<std::uint32_t>(height);
    const std::uint32_t fileSize = 14 + 40 + dataSize;

    std::vector<std::uint8_t> file(fileSize, 0);
    auto put16 = [&](std::size_t off, std::uint16_t v) {
        file[off] = static_cast<std::uint8_t>(v & 0xFF);
        file[off + 1] = static_cast<std::uint8_t>(v >> 8);
    };
    auto put32 = [&](std::size_t off, std::uint32_t v) {
        file[off] = static_cast<std::uint8_t>(v & 0xFF);
        file[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
        file[off + 2] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
        file[off + 3] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
    };

    put16(0, 0x4D42);            // 'BM'
    put32(2, fileSize);
    put32(10, 14 + 40);          // pixel data offset
    put32(14, 40);               // BITMAPINFOHEADER size
    put32(18, static_cast<std::uint32_t>(width));
    put32(22, static_cast<std::uint32_t>(height));
    put16(26, 1);                // planes
    put16(28, 24);               // bits per pixel
    put32(30, 0);                // BI_RGB
    put32(34, dataSize);

    for (int y = 0; y < height; ++y) {
        const std::size_t srcRow = static_cast<std::size_t>(y) * rowBytes;
        std::uint8_t* dst = file.data() + 14 + 40 +
                            static_cast<std::size_t>(y) * paddedRow;
        for (int x = 0; x < width; ++x) {
            dst[x * 3 + 0] = rgb[srcRow + static_cast<std::size_t>(x) * 3 + 2]; // B
            dst[x * 3 + 1] = rgb[srcRow + static_cast<std::size_t>(x) * 3 + 1]; // G
            dst[x * 3 + 2] = rgb[srcRow + static_cast<std::size_t>(x) * 3 + 0]; // R
        }
    }

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const bool ok = std::fwrite(file.data(), 1, file.size(), f) == file.size();
    std::fclose(f);
    return ok;
}

} // namespace maze::debug
