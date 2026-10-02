// Texture.hpp - OpenGL texture RAII wrapper + procedural texture factory.
//
// The game ships no binary assets: wall, floor and ceiling textures are
// synthesised on the CPU at startup from hash noise, then uploaded once.
#pragma once

#include <cstdint>
#include <functional>

#include <glad/gl.h>

namespace maze {

class Texture {
public:
    using PixelFn = std::function<void(int x, int y, std::uint8_t out[4])>;

    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Builds an RGBA8 texture, generates mipmaps and sets REPEAT wrap +
    // trilinear filtering with anisotropic filtering when available.
    static Texture createRgba(int width, int height, const PixelFn& generator);

    // Builds a single-channel (GL_R8) texture with NEAREST filtering and
    // clamp-to-edge wrap - used by the bitmap-font HUD atlas. No mipmaps.
    static Texture createR8(int width, int height, const std::uint8_t* pixels);

    // Procedural materials (deterministic - identical on every machine).
    static Texture makeWallBricks();
    static Texture makeFloorStones();
    static Texture makeCeiling();

    GLuint id() const { return texture_; }
    void bind(GLuint unit) const;

private:
    GLuint texture_ = 0;
};

} // namespace maze
