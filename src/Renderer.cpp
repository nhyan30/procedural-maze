#include "Renderer.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

#include "Math.hpp"

namespace maze {
namespace {

// Unit cube: X/Z in [-0.5, 0.5], Y in [0, 1] (base sits on the floor).
// Each face stores 4 corners; triangles (0,1,2) and (0,2,3) wind CCW when
// viewed from outside so backface culling works with the default front face.
struct FaceDef {
    vec3 v[4];
    vec3 normal;
};

constexpr FaceDef kCubeFaces[6] = {
    { { vec3( 0.5f, 0.0f,  0.5f), vec3( 0.5f, 0.0f, -0.5f), vec3( 0.5f, 1.0f, -0.5f), vec3( 0.5f, 1.0f,  0.5f) }, vec3( 1.0f, 0.0f, 0.0f) },
    { { vec3(-0.5f, 0.0f, -0.5f), vec3(-0.5f, 0.0f,  0.5f), vec3(-0.5f, 1.0f,  0.5f), vec3(-0.5f, 1.0f, -0.5f) }, vec3(-1.0f, 0.0f, 0.0f) },
    { { vec3(-0.5f, 1.0f,  0.5f), vec3( 0.5f, 1.0f,  0.5f), vec3( 0.5f, 1.0f, -0.5f), vec3(-0.5f, 1.0f, -0.5f) }, vec3( 0.0f, 1.0f, 0.0f) },
    { { vec3(-0.5f, 0.0f, -0.5f), vec3( 0.5f, 0.0f, -0.5f), vec3( 0.5f, 0.0f,  0.5f), vec3(-0.5f, 0.0f,  0.5f) }, vec3( 0.0f,-1.0f, 0.0f) },
    { { vec3(-0.5f, 0.0f,  0.5f), vec3( 0.5f, 0.0f,  0.5f), vec3( 0.5f, 1.0f,  0.5f), vec3(-0.5f, 1.0f,  0.5f) }, vec3( 0.0f, 0.0f, 1.0f) },
    { { vec3( 0.5f, 0.0f, -0.5f), vec3(-0.5f, 0.0f, -0.5f), vec3(-0.5f, 1.0f, -0.5f), vec3( 0.5f, 1.0f, -0.5f) }, vec3( 0.0f, 0.0f,-1.0f) },
};

constexpr float kFaceUV[4][2] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

void pushQuad(std::vector<float>& out, const vec3 v[4], const vec3& normal,
              const float uv[4][2]) {
    const int tris[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i : tris) {
        out.push_back(v[i].x); out.push_back(v[i].y); out.push_back(v[i].z);
        out.push_back(normal.x); out.push_back(normal.y); out.push_back(normal.z);
        out.push_back(uv[i][0]); out.push_back(uv[i][1]);
    }
}

GLuint createVao() {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    return vao;
}

GLuint createVbo(GLsizeiptr size, const void* data, GLenum usage) {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
}

// Standard 3-float position / 3-float normal / 2-float uv layout.
void bindVertexNormalUvAttribs(GLuint vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    constexpr GLsizei kStride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kStride, reinterpret_cast<void*>(6 * sizeof(float)));
}

void bindPosOnlyAttrib(GLuint vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
}

} // namespace

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
Renderer::Renderer(const std::string& shaderDir)
    : mazeProg_(shaderDir + "/maze.vert", shaderDir + "/maze.frag"),
      flatProg_(shaderDir + "/flat.vert", shaderDir + "/flat.frag"),
      wallTex_(Texture::makeWallBricks()),
      floorTex_(Texture::makeFloorStones()),
      ceilTex_(Texture::makeCeiling()) {
    createStaticGeometry();
    glEnable(GL_MULTISAMPLE);
}

Renderer::~Renderer() { releaseGpuObjects(); }

void Renderer::releaseGpuObjects() {
    glDeleteVertexArrays(1, &cubeVAO_);
    glDeleteBuffers(1, &cubeVBO_);
    glDeleteBuffers(1, &instanceVBO_);
    glDeleteVertexArrays(1, &floorVAO_);
    glDeleteBuffers(1, &floorVBO_);
    glDeleteVertexArrays(1, &ceilVAO_);
    glDeleteBuffers(1, &ceilVBO_);
    glDeleteVertexArrays(1, &ndcQuadVAO_);
    glDeleteBuffers(1, &ndcQuadVBO_);
    glDeleteVertexArrays(1, &miniWallsVAO_);
    glDeleteBuffers(1, &miniWallsVBO_);
    glDeleteVertexArrays(1, &markersVAO_);
    glDeleteBuffers(1, &markersVBO_);
}

void Renderer::createStaticGeometry() {
    // --- Instanced cube ------------------------------------------------------
    std::vector<float> cube;
    cube.reserve(36 * 8);
    for (const FaceDef& face : kCubeFaces)
        pushQuad(cube, face.v, face.normal, kFaceUV);

    cubeVAO_ = createVao();
    cubeVBO_ = createVbo(static_cast<GLsizeiptr>(cube.size() * sizeof(float)),
                         cube.data(), GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO_);
    bindVertexNormalUvAttribs(cubeVBO_);

    glGenBuffers(1, &instanceVBO_); // filled by build()
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);
    glEnableVertexAttribArray(3); // vec4: world offset xyz + uv scale
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glVertexAttribDivisor(3, 1);

    // --- Floor / ceiling quads (world scale comes from uModelScale) ----------
    const float uv[4][2] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };

    const vec3 floorVerts[4] = {
        vec3(-0.5f, 0.0f,  0.5f), vec3(0.5f, 0.0f,  0.5f),
        vec3( 0.5f, 0.0f, -0.5f), vec3(-0.5f, 0.0f, -0.5f) };   // normal +Y
    const vec3 ceilVerts[4] = {
        vec3(-0.5f, 0.0f, -0.5f), vec3(0.5f, 0.0f, -0.5f),
        vec3( 0.5f, 0.0f,  0.5f), vec3(-0.5f, 0.0f,  0.5f) };   // normal -Y

    std::vector<float> floorData, ceilData;
    floorData.reserve(6 * 8);
    ceilData.reserve(6 * 8);
    pushQuad(floorData, floorVerts, vec3(0.0f, 1.0f, 0.0f), uv);
    pushQuad(ceilData, ceilVerts, vec3(0.0f, -1.0f, 0.0f), uv);

    floorVAO_ = createVao();
    floorVBO_ = createVbo(static_cast<GLsizeiptr>(floorData.size() * sizeof(float)),
                          floorData.data(), GL_STATIC_DRAW);
    glBindVertexArray(floorVAO_);
    bindVertexNormalUvAttribs(floorVBO_);

    ceilVAO_ = createVao();
    ceilVBO_ = createVbo(static_cast<GLsizeiptr>(ceilData.size() * sizeof(float)),
                         ceilData.data(), GL_STATIC_DRAW);
    glBindVertexArray(ceilVAO_);
    bindVertexNormalUvAttribs(ceilVBO_);

    // --- Fullscreen NDC quad (fade overlay, minimap background) ---------------
    const float ndc[6][3] = {
        { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f }, { 1.0f,  1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f } };
    ndcQuadVAO_ = createVao();
    ndcQuadVBO_ = createVbo(sizeof(ndc), ndc, GL_STATIC_DRAW);
    glBindVertexArray(ndcQuadVAO_);
    bindPosOnlyAttrib(ndcQuadVBO_);

    // --- Minimap wall batch (rebuilt per maze) + dynamic markers --------------
    miniWallsVAO_ = createVao();
    glBindVertexArray(miniWallsVAO_);
    glGenBuffers(1, &miniWallsVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, miniWallsVBO_);
    bindPosOnlyAttrib(miniWallsVBO_); // storage (re)allocated in build()

    markersVAO_ = createVao();
    markersVBO_ = createVbo(9 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glBindVertexArray(markersVAO_);
    bindPosOnlyAttrib(markersVBO_);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::build(const Maze& maze) {
    const float s = cfg::kCellSize;
    const float minX = maze.worldMinX();
    const float minZ = maze.worldMinZ();
    const int g = maze.gridSize();

    // Only wall cells that touch a corridor can ever be visible; enclosed
    // cells are skipped, which removes ~30-40% of instances on typical mazes.
    std::vector<float> instances;
    std::vector<float> miniWalls;
    const int maxWalls = g * g;
    instances.reserve(static_cast<std::size_t>(maxWalls) * 4);
    miniWalls.reserve(static_cast<std::size_t>(maxWalls) * 6 * 3);

    constexpr int kDr[4] = { -1, 1, 0, 0 };
    constexpr int kDc[4] = { 0, 0, -1, 1 };

    for (int row = 0; row < g; ++row) {
        for (int col = 0; col < g; ++col) {
            if (!maze.isWall(row, col)) continue;

            bool touchesCorridor = false;
            for (int i = 0; i < 4; ++i) {
                if (!maze.isWall(row + kDr[i], col + kDc[i])) {
                    touchesCorridor = true;
                    break;
                }
            }
            if (!touchesCorridor) continue;

            const float cx = minX + (static_cast<float>(col) + 0.5f) * s;
            const float cz = minZ + (static_cast<float>(row) + 0.5f) * s;
            instances.push_back(cx);
            instances.push_back(0.0f);
            instances.push_back(cz);
            instances.push_back(1.0f); // uv scale: one brick tile per cell face

            // Minimap quad, stored as (worldX, worldZ, 0) pairs.
            const float x0 = minX + static_cast<float>(col) * s;
            const float z0 = minZ + static_cast<float>(row) * s;
            const float x1 = x0 + s;
            const float z1 = z0 + s;
            const float quad[6][3] = {
                { x0, z0, 0.0f }, { x1, z0, 0.0f }, { x1, z1, 0.0f },
                { x0, z0, 0.0f }, { x1, z1, 0.0f }, { x0, z1, 0.0f } };
            for (const auto& v : quad) {
                miniWalls.push_back(v[0]);
                miniWalls.push_back(v[1]);
                miniWalls.push_back(v[2]);
            }
        }
    }

    wallInstanceCount_ = static_cast<GLsizei>(instances.size() / 4);
    miniWallVertCount_ = static_cast<GLsizei>(miniWalls.size() / 3);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(instances.size() * sizeof(float)),
                 instances.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, miniWallsVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(miniWalls.size() * sizeof(float)),
                 miniWalls.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    halfSpan_ = maze.worldSpan() * 0.5f;
    const auto [ex, ez] = maze.cellCenter(maze.exitCell());
    exitCenter_ = vec3(ex, 0.0f, ez);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
void Renderer::drawFrame(const FrameState& fs) {
    if (fs.viewportWidth <= 0 || fs.viewportHeight <= 0 || !fs.camera) return;

    glViewport(0, 0, fs.viewportWidth, fs.viewportHeight);
    glClearColor(cfg::kClearColor[0], cfg::kClearColor[1], cfg::kClearColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    const float aspect = static_cast<float>(fs.viewportWidth) /
                         static_cast<float>(fs.viewportHeight);
    const mat4 viewProj = fs.camera->projection(aspect) * fs.camera->view();

    drawWorld(fs, viewProj);
    drawBeacon(fs, viewProj);
    if (fs.showMinimap) drawMinimap(fs);
    drawFade(fs);

    glBindVertexArray(0);
    glViewport(0, 0, fs.viewportWidth, fs.viewportHeight);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// World pass: floor, ceiling, instanced walls - one lit shader, three draws.
// ---------------------------------------------------------------------------
void Renderer::drawWorld(const FrameState& fs, const mat4& viewProj) {
    mazeProg_.bind();
    mazeProg_.setMat4("uViewProj", viewProj);
    mazeProg_.setVec3("uCamPos", fs.camera->position.x, fs.camera->position.y,
                      fs.camera->position.z);

    mazeProg_.setVec3("uAmbient", cfg::kAmbient);
    const vec3 moonDir = normalize(vec3(cfg::kMoonDirection[0], cfg::kMoonDirection[1],
                                        cfg::kMoonDirection[2]));
    mazeProg_.setVec3("uDirDir", moonDir.x, moonDir.y, moonDir.z);
    mazeProg_.setVec3("uDirColor", cfg::kMoonColor);

    // Point light 0 = player torch, 1 = exit beacon.
    mazeProg_.setVec3("uPointPos[0]", fs.torchPos.x, fs.torchPos.y, fs.torchPos.z);
    mazeProg_.setVec3("uPointPos[1]", fs.beaconPos.x, fs.beaconPos.y, fs.beaconPos.z);
    mazeProg_.setVec3("uPointColor[0]", cfg::kTorchColor);
    mazeProg_.setVec3("uPointColor[1]", cfg::kBeaconColor);
    mazeProg_.setFloat("uPointIntensity[0]", fs.torchPower);
    mazeProg_.setFloat("uPointIntensity[1]", fs.beaconPower);
    mazeProg_.setVec3("uPointAtten[0]", cfg::kTorchAtten);
    mazeProg_.setVec3("uPointAtten[1]", cfg::kBeaconAtten);

    mazeProg_.setVec3("uFogColor", cfg::kFogColor);
    mazeProg_.setFloat("uFogDensity", cfg::kFogDensity);

    // Floor: non-instanced quad. Attribute 3 (instance data) is disabled for
    // this VAO, so the current attribute value provides offset + uv scale.
    wallTex_.bind(0);
    floorTex_.bind(1);
    ceilTex_.bind(2);

    const float span = halfSpan_ * 2.0f;

    mazeProg_.setVec3("uModelScale", span, 0.0f, span);
    mazeProg_.setInt("uTex", 1);
    mazeProg_.setFloat("uSpecular", cfg::kFloorSpecular);
    mazeProg_.setFloat("uShininess", cfg::kFloorShininess);
    glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, cfg::kCellSize); // uv: one tile / 2 m
    glBindVertexArray(floorVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Ceiling: same quad, shifted up to wall height via the instance offset.
    mazeProg_.setInt("uTex", 2);
    mazeProg_.setFloat("uSpecular", cfg::kCeilSpecular);
    mazeProg_.setFloat("uShininess", cfg::kCeilShininess);
    glVertexAttrib4f(3, 0.0f, cfg::kWallHeight, 0.0f, cfg::kCellSize);
    glBindVertexArray(ceilVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Walls: one instanced draw call for every visible wall cell.
    mazeProg_.setVec3("uModelScale", cfg::kCellSize, cfg::kWallHeight, cfg::kCellSize);
    mazeProg_.setInt("uTex", 0);
    mazeProg_.setFloat("uSpecular", cfg::kWallSpecular);
    mazeProg_.setFloat("uShininess", cfg::kWallShininess);
    glBindVertexArray(cubeVAO_);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, wallInstanceCount_);
}

// ---------------------------------------------------------------------------
// Beacon: unlit emissive pillar at the exit, pulsing gently. It is drawn
// without fog so it reads as a light source rather than a surface.
// ---------------------------------------------------------------------------
void Renderer::drawBeacon(const FrameState& fs, const mat4& viewProj) {
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(fs.time) *
                                               cfg::kBeaconPulseSpeed);
    const float height = 1.45f + 0.25f * pulse;
    const float width = cfg::kBeaconRadius * 2.0f;

    const mat4 model = mat4::translate(vec3(exitCenter_.x, 0.0f, exitCenter_.z)) *
                       mat4::scale(vec3(width, height, width));

    flatProg_.bind();
    flatProg_.setMat4("uMVP", viewProj * model);
    const float glow = 0.75f + 0.45f * pulse;
    flatProg_.setVec4("uColor", cfg::kBeaconColor[0] * glow, cfg::kBeaconColor[1] * glow,
                      cfg::kBeaconColor[2] * glow, 1.0f);

    glBindVertexArray(cubeVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// ---------------------------------------------------------------------------
// Minimap: orthographic top-down pass into a corner of the screen. Geometry
// is pre-built in (worldX, worldZ, 0) form; a reversed top/bottom ortho flips
// world Z so maze row 0 ends up at the top of the map.
// ---------------------------------------------------------------------------
void Renderer::drawMinimap(const FrameState& fs) {
    const int size = static_cast<int>(static_cast<float>(
        std::min(fs.viewportWidth, fs.viewportHeight)) * cfg::kMinimapScreenFraction);
    if (size < 32) return;

    const int ox = fs.viewportWidth - size - cfg::kMinimapMarginPx;
    const int oy = fs.viewportHeight - size - cfg::kMinimapMarginPx;

    glViewport(ox, oy, size, size);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    flatProg_.bind();

    // Background.
    flatProg_.setMat4("uMVP", mat4::identity());
    flatProg_.setVec4("uColor", 0.02f, 0.025f, 0.035f, cfg::kMinimapBgAlpha);
    glBindVertexArray(ndcQuadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    const mat4 ortho = mat4::ortho(-halfSpan_, halfSpan_, halfSpan_, -halfSpan_,
                                   -1.0f, 1.0f);

    // Wall cells (batched, rebuilt on maze change).
    if (miniWallVertCount_ > 0) {
        flatProg_.setMat4("uMVP", ortho);
        flatProg_.setVec4("uColor", 0.52f, 0.55f, 0.60f, 0.92f);
        glBindVertexArray(miniWallsVAO_);
        glDrawArrays(GL_TRIANGLES, 0, miniWallVertCount_);
    }

    // Player triangle + exit marker, rebuilt every frame (9 vertices total).
    // Sizes are in metres, chosen to stay legible at typical minimap scales.
    const vec3 fwd = fs.camera->forwardFlat();
    const vec3 right = fs.camera->right();
    const vec3 p = fs.camera->position;
    const float verts[9][3] = {
        // Player triangle: tip ahead, two base corners behind.
        { p.x + fwd.x * 1.30f,                   p.z + fwd.z * 1.30f,                   0.0f },
        { p.x - fwd.x * 0.55f + right.x * 0.60f, p.z - fwd.z * 0.55f + right.z * 0.60f, 0.0f },
        { p.x - fwd.x * 0.55f - right.x * 0.60f, p.z - fwd.z * 0.55f - right.z * 0.60f, 0.0f },
        // Exit marker quad.
        { exitCenter_.x - 0.60f, exitCenter_.z - 0.60f, 0.0f },
        { exitCenter_.x + 0.60f, exitCenter_.z - 0.60f, 0.0f },
        { exitCenter_.x + 0.60f, exitCenter_.z + 0.60f, 0.0f },
        { exitCenter_.x - 0.60f, exitCenter_.z - 0.60f, 0.0f },
        { exitCenter_.x + 0.60f, exitCenter_.z + 0.60f, 0.0f },
        { exitCenter_.x - 0.60f, exitCenter_.z + 0.60f, 0.0f },
    };
    glBindBuffer(GL_ARRAY_BUFFER, markersVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    flatProg_.setMat4("uMVP", ortho);
    glBindVertexArray(markersVAO_);

    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(fs.time) *
                                               cfg::kBeaconPulseSpeed);
    flatProg_.setVec4("uColor", 1.0f, 0.80f, 0.35f, 1.0f); // player
    glDrawArrays(GL_TRIANGLES, 0, 3);
    flatProg_.setVec4("uColor", cfg::kBeaconColor[0], cfg::kBeaconColor[1],
                      cfg::kBeaconColor[2], 0.45f + 0.55f * pulse); // exit
    glDrawArrays(GL_TRIANGLES, 3, 6);

    // Restore main-view state.
    glViewport(0, 0, fs.viewportWidth, fs.viewportHeight);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

// ---------------------------------------------------------------------------
// Fade overlay: fullscreen white quad whose alpha drives the win sequence.
// ---------------------------------------------------------------------------
void Renderer::drawFade(const FrameState& fs) {
    if (fs.whiteFade <= 0.0f) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    flatProg_.bind();
    flatProg_.setMat4("uMVP", mat4::identity());
    flatProg_.setVec4("uColor", 1.0f, 1.0f, 1.0f, clampf(fs.whiteFade, 0.0f, 1.0f));

    glBindVertexArray(ndcQuadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

} // namespace maze
