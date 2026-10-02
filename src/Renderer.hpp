// Renderer.hpp - all OpenGL drawing: world, beacon, minimap, fade overlay.
//
// Layout:
//   * Walls      - one unit cube, one instanced draw call (1 instance per
//                  visible wall cell). Only wall cells that touch a corridor
//                  are uploaded; fully enclosed cells can never be seen.
//   * Floor/ceil - single quads through the same lit shader. They are not
//                  instanced: the per-instance attribute (location 3) is left
//                  disabled for their VAOs and fed from the current attribute
//                  value (glVertexAttrib4f) instead, so no shader branch and
//                  no extra program is needed.
//   * Beacon     - unlit emissive cube, visible down long corridors.
//   * Minimap    - second orthographic pass into a corner of the viewport:
//                  background quad, batched wall cells, player triangle and
//                  exit marker.
//   * HUD        - pixel-space controls hint in the bottom-left corner:
//                  translucent panel plus 1-bit bitmap-font text (two
//                  dynamic batches: solid panel, glyph quads).
//   * Fade       - fullscreen NDC quad used by the exit sequence.
#pragma once

#include <string>
#include <vector>

#include <glad/gl.h>

#include "Camera.hpp"
#include "Config.hpp"
#include "Maze.hpp"
#include "Shader.hpp"
#include "Texture.hpp"

namespace maze {

struct FrameState {
    const Camera* camera = nullptr;
    int viewportWidth = 0;
    int viewportHeight = 0;

    vec3 torchPos;       // player-attached point light
    float torchPower = 0.0f;
    vec3 beaconPos;      // exit point light
    float beaconPower = 0.0f;

    bool showMinimap = false;
    float whiteFade = 0.0f; // 0 = off .. 1 = fully white
    double time = 0.0;
};

class Renderer {
public:
    explicit Renderer(const std::string& shaderDir);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Uploads per-maze GPU data (wall instances, minimap batch). Called once
    // at startup and again whenever the maze is (re)generated.
    void build(const Maze& maze);

    void drawFrame(const FrameState& fs);

private:
    void createStaticGeometry();
    void createFontAtlas();
    void createHudGeometry();
    void releaseGpuObjects();

    void drawWorld(const FrameState& fs, const mat4& viewProj);
    void drawBeacon(const FrameState& fs, const mat4& viewProj);
    void drawMinimap(const FrameState& fs);
    void drawHud(const FrameState& fs);
    void drawFade(const FrameState& fs);

    // HUD batching (pixel-space, origin top-left). One struct per vertex:
    // position + atlas UV + RGBA tint.
    struct HudVertex {
        float x, y, u, v;
        float r, g, b, a;
    };
    void hudClear();
    void hudQuad(float x0, float y0, float x1, float y1, float u0, float v0,
                 float u1, float v1, float r, float g, float b, float a);
    void hudSolidRect(float x, float y, float w, float h, const float rgb[3],
                      float a);
    // Draws "key" + gap + "label" starting at x; returns the pen x past it.
    float hudEntry(const char* key, const char* label, float x, float y);
    void uploadHudBatch(float solid);

    Shader mazeProg_;
    Shader flatProg_;
    Shader hudProg_;
    Texture wallTex_;
    Texture floorTex_;
    Texture ceilTex_;
    Texture fontTex_;       // 1-bit glyph atlas for the HUD (createR8)

    GLuint hudVAO_ = 0;
    GLuint hudVBO_ = 0;
    std::vector<HudVertex> hudVerts_;

    GLuint cubeVAO_ = 0;
    GLuint cubeVBO_ = 0;
    GLuint instanceVBO_ = 0;
    GLuint floorVAO_ = 0;
    GLuint floorVBO_ = 0;
    GLuint ceilVAO_ = 0;
    GLuint ceilVBO_ = 0;
    GLuint ndcQuadVAO_ = 0;
    GLuint ndcQuadVBO_ = 0;
    GLuint miniWallsVAO_ = 0;
    GLuint miniWallsVBO_ = 0;
    GLuint markersVAO_ = 0;
    GLuint markersVBO_ = 0;

    GLsizei wallInstanceCount_ = 0;
    GLsizei miniWallVertCount_ = 0;

    float halfSpan_ = 0.0f;    // half of the maze world extent
    vec3 exitCenter_{};
};

} // namespace maze
