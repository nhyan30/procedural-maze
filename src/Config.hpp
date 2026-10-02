// Config.hpp - central place for every gameplay/visual tuning constant.
//
// All gameplay and lighting numbers live here so behaviour can be adjusted
// (or exposed via CLI later) without digging through the systems.
#pragma once

#include <cstdint>

namespace maze::cfg {

// --- World scale -----------------------------------------------------------
inline constexpr float kCellSize   = 2.0f;  // metres per grid cell
inline constexpr float kWallHeight = 2.75f; // wall / ceiling height

// --- Player ----------------------------------------------------------------
inline constexpr float kEyeHeight       = 1.55f;
inline constexpr float kPlayerRadius    = 0.34f;
inline constexpr float kWalkSpeed       = 3.4f;  // m/s
inline constexpr float kSprintSpeed     = 5.4f;  // m/s while holding Shift
inline constexpr float kMoveAccel       = 11.0f; // exponential accel blend rate
inline constexpr float kMaxPhysicsStep  = 1.0f / 60.0f; // collision substep
inline constexpr float kMouseSensitivity = 0.0021f;    // rad per pixel
inline constexpr float kMaxPitch        = 1.5533f;      // ~89 degrees

// --- Camera ----------------------------------------------------------------
inline constexpr float kFovDegrees = 72.0f;
inline constexpr float kZNear      = 0.05f;
inline constexpr float kZFar       = 120.0f;

// --- Maze ------------------------------------------------------------------
inline constexpr int      kDefaultCellsPerSide = 10; // corridor cells; grid = 2n+1
inline constexpr int      kMinCellsPerSide     = 4;
inline constexpr int      kMaxCellsPerSide     = 64;
inline constexpr float    kBraidProbability    = 0.22f; // dead-end removal chance
inline constexpr uint64_t kDefaultSeedFallback = 0x9E3779B97F4A7C15ull;

// --- Lighting --------------------------------------------------------------
// Ambient "sky" term plus a faint cool directional light keep unlit areas
// barely readable while the warm torch dominates near the player.
inline constexpr float kAmbient[3]       = { 0.012f, 0.013f, 0.018f };
inline constexpr float kMoonDirection[3] = { 0.42f, -0.84f, 0.34f }; // travel dir
inline constexpr float kMoonColor[3]     = { 0.030f, 0.038f, 0.055f };

inline constexpr float kTorchColor[3]    = { 1.00f, 0.62f, 0.32f };
inline constexpr float kTorchBasePower   = 1.70f;
inline constexpr float kTorchFlickerAmp  = 0.11f; // +- fraction around base
inline constexpr float kTorchAtten[3]    = { 1.00f, 0.22f, 0.180f };

inline constexpr float kBeaconColor[3]   = { 0.30f, 0.95f, 0.80f };
inline constexpr float kBeaconBasePower  = 0.90f;
inline constexpr float kBeaconAtten[3]   = { 1.00f, 0.45f, 0.70f };
inline constexpr float kBeaconPulseSpeed = 2.2f;  // rad/s
inline constexpr float kBeaconRadius     = 0.22f; // pillar half-width

// --- Fog -------------------------------------------------------------------
inline constexpr float kFogDensity    = 0.110f;
inline constexpr float kFogColor[3]   = { 0.012f, 0.014f, 0.020f };
inline constexpr float kClearColor[3] = { 0.012f, 0.014f, 0.020f };

// --- Material response (per draw call) --------------------------------------
// Specular strength / Blinn-Phong exponent for walls, floor, ceiling.
inline constexpr float kWallSpecular   = 0.22f;
inline constexpr float kWallShininess  = 24.0f;
inline constexpr float kFloorSpecular  = 0.55f;
inline constexpr float kFloorShininess = 42.0f;
inline constexpr float kCeilSpecular   = 0.06f;
inline constexpr float kCeilShininess  = 12.0f;

// --- Exit sequence ----------------------------------------------------------
inline constexpr float kWinFadeOutSeconds = 1.4f; // white ramp while flaring
inline constexpr float kWinFadeInSeconds  = 0.9f; // fade back after reroll
inline constexpr float kWinBeaconFlare    = 7.0f; // extra light power at full

// --- Minimap ----------------------------------------------------------------
inline constexpr float kMinimapScreenFraction = 0.30f; // of smaller viewport axis
inline constexpr int   kMinimapMarginPx       = 14;
inline constexpr float kMinimapBgAlpha        = 0.88f;

// --- Window -----------------------------------------------------------------
inline constexpr int kInitialWidth  = 1280;
inline constexpr int kInitialHeight = 720;

} // namespace maze::cfg
