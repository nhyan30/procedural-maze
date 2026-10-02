#include "Texture.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace maze {
namespace {

// ---------------------------------------------------------------------------
// Deterministic hash / value-noise toolkit. No global state, so textures are
// bit-identical across runs and machines.
// ---------------------------------------------------------------------------
std::uint32_t hash2d(std::uint32_t x, std::uint32_t y, std::uint32_t seed) {
    std::uint32_t h = x * 374761393u + y * 668265263u + seed * 1442695041u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    return h ^ (h >> 16u);
}

float rand01(std::uint32_t h) { return static_cast<float>(h & 0xFFFFFFu) / 16777215.0f; }

float smoothstep(float t) { return t * t * (3.0f - 2.0f * t); }

// Bilinear value noise on an integer lattice.
float valueNoise(float x, float y, std::uint32_t seed) {
    const float xf = std::floor(x), yf = std::floor(y);
    const int xi = static_cast<int>(xf), yi = static_cast<int>(yf);
    const float tx = smoothstep(x - xf), ty = smoothstep(y - yf);

    const float v00 = rand01(hash2d(static_cast<std::uint32_t>(xi),     static_cast<std::uint32_t>(yi),     seed));
    const float v10 = rand01(hash2d(static_cast<std::uint32_t>(xi + 1), static_cast<std::uint32_t>(yi),     seed));
    const float v01 = rand01(hash2d(static_cast<std::uint32_t>(xi),     static_cast<std::uint32_t>(yi + 1), seed));
    const float v11 = rand01(hash2d(static_cast<std::uint32_t>(xi + 1), static_cast<std::uint32_t>(yi + 1), seed));

    const float a = v00 + (v10 - v00) * tx;
    const float b = v01 + (v11 - v01) * tx;
    return a + (b - a) * ty;
}

// Two-octave fractal noise; period scales are in texture pixels.
float fbm(float x, float y, std::uint32_t seed) {
    float sum = 0.0f;
    sum += 0.60f * valueNoise(x / 64.0f, y / 64.0f, seed);
    sum += 0.30f * valueNoise(x / 23.0f, y / 23.0f, seed * 7u + 1u);
    sum += 0.10f * valueNoise(x / 7.0f,  y / 7.0f,  seed * 13u + 2u);
    return sum; // ~[0, 1]
}

// Writes one RGBA pixel: `out` points at the 4-byte slot inside the buffer.
void writePixel(std::uint8_t* out, float r, float g, float b) {
    out[0] = static_cast<std::uint8_t>(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
    out[1] = static_cast<std::uint8_t>(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
    out[2] = static_cast<std::uint8_t>(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);
    out[3] = 255;
}

} // namespace

// ---------------------------------------------------------------------------
// Texture object
// ---------------------------------------------------------------------------
Texture::~Texture() {
    if (texture_ != 0) glDeleteTextures(1, &texture_);
}

Texture::Texture(Texture&& other) noexcept : texture_(other.texture_) {
    other.texture_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (texture_ != 0) glDeleteTextures(1, &texture_);
        texture_ = other.texture_;
        other.texture_ = 0;
    }
    return *this;
}

Texture Texture::createRgba(int width, int height, const PixelFn& generator) {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            generator(x, y, &pixels[(static_cast<std::size_t>(y) * width + x) * 4]);

    Texture t;
    glGenTextures(1, &t.texture_);
    glBindTexture(GL_TEXTURE_2D, t.texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Anisotropy keeps wall textures legible at grazing angles down corridors.
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    if (maxAniso > 1.0f)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        std::min(maxAniso, 8.0f));
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

void Texture::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_);
}

// ---------------------------------------------------------------------------
// Wall: offset brick course with mortar, per-brick shade variance, edge AO
// and large-scale grime. One texture tile covers one 2 m wall face.
// ---------------------------------------------------------------------------
Texture Texture::makeWallBricks() {
    constexpr int kSize = 512;
    constexpr int kRows = 8, kRowH = kSize / kRows;   // 64 px
    constexpr int kCols = 4, kColW = kSize / kCols;   // 128 px
    constexpr int kMortar = 5;
    constexpr std::uint32_t kSeed = 0xB10C4u;

    return createRgba(kSize, kSize, [](int px, int py, std::uint8_t* out) {
        const int row  = py / kRowH;
        const int xoff = (row % 2) * (kColW / 2);
        const int xb   = (px + xoff) % kColW;
        const int yb   = py % kRowH;

        // Coordinates of this pixel inside the (offset) brick grid.
        const int brickCol = ((px + xoff) / kColW) % kCols;
        const int ex = std::min(xb, kColW - 1 - xb); // distance to mortar line
        const int ey = std::min(yb, kRowH - 1 - yb);

        const float grime = fbm(static_cast<float>(px), static_cast<float>(py), kSeed);

        if (ex < kMortar || ey < kMortar) {
            const float n = 0.90f + 0.20f * grime;
            writePixel(out, 0.042f * n, 0.040f * n, 0.036f * n);
            return;
        }

        const float brickShade = 0.62f + 0.56f * rand01(hash2d(
            static_cast<std::uint32_t>(brickCol), static_cast<std::uint32_t>(row), kSeed));
        const float grain = 0.80f + 0.40f * grime;

        // Darken towards mortar edges: cheap baked ambient occlusion.
        const float edge = smoothstep(std::clamp(
            static_cast<float>(std::min(ex, ey)) / 8.0f, 0.0f, 1.0f));
        const float ao = 0.45f + 0.55f * edge;

        writePixel(out,
                   0.440f * brickShade * grain * ao,
                   0.290f * brickShade * grain * ao,
                   0.210f * brickShade * grain * ao);
    });
}

// ---------------------------------------------------------------------------
// Floor: large stone tiles, slightly cooler and darker than the walls.
// ---------------------------------------------------------------------------
Texture Texture::makeFloorStones() {
    constexpr int kSize = 512;
    constexpr int kTiles = 4, kTile = kSize / kTiles; // 128 px
    constexpr int kGrout = 4;
    constexpr std::uint32_t kSeed = 0x5704E5u;

    return createRgba(kSize, kSize, [](int px, int py, std::uint8_t* out) {
        const int col = px / kTile;
        const int row = py / kTile;
        const int xt  = px % kTile;
        const int yt  = py % kTile;
        const int ex  = std::min(xt, kTile - 1 - xt);
        const int ey  = std::min(yt, kTile - 1 - yt);

        const float grain = fbm(static_cast<float>(px), static_cast<float>(py), kSeed);

        if (ex < kGrout || ey < kGrout) {
            const float n = 0.85f + 0.30f * grain;
            writePixel(out, 0.040f * n, 0.038f * n, 0.036f * n);
            return;
        }

        const float tileShade = 0.72f + 0.46f * rand01(hash2d(
            static_cast<std::uint32_t>(col), static_cast<std::uint32_t>(row), kSeed));

        const float edge = smoothstep(std::clamp(
            static_cast<float>(std::min(ex, ey)) / 14.0f, 0.0f, 1.0f));
        const float ao = 0.62f + 0.38f * edge;
        const float detail = 0.94f + 0.12f * grain;

        writePixel(out,
                   0.300f * tileShade * detail * ao,
                   0.270f * tileShade * detail * ao,
                   0.240f * tileShade * detail * ao);
    });
}

// ---------------------------------------------------------------------------
// Ceiling: dark rough rock with broad tonal blotches - intentionally dull so
// the torch reads as the dominant light source.
// ---------------------------------------------------------------------------
Texture Texture::makeCeiling() {
    constexpr int kSize = 256;
    constexpr std::uint32_t kSeed = 0xCE171u;

    return createRgba(kSize, kSize, [](int px, int py, std::uint8_t* out) {
        const float blotch = fbm(static_cast<float>(px) * 1.6f,
                                 static_cast<float>(py) * 1.6f, kSeed);
        const float shade = 0.82f + 0.36f * blotch;
        writePixel(out, 0.120f * shade, 0.115f * shade, 0.110f * shade);
    });
}

} // namespace maze
