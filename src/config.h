#pragma once

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TARGET_FPS = 240;

constexpr float PLAYER_MOVE_SPEED = 6.0f;
constexpr float PLAYER_MOUSE_SENSITIVITY = 0.003f;
constexpr float PLAYER_GRAVITY = -25.0f;
constexpr float PLAYER_JUMP_FORCE = 9.0f;
constexpr float PLAYER_HEIGHT = 1.8f;
constexpr float PLAYER_RADIUS = 0.3f;

// CHUNK_SIZE = 4 гарантирует отсутствие переполнения unsigned short
constexpr int CHUNK_SIZE = 4; 
constexpr int CHUNK_HEIGHT = 64;
constexpr int RENDER_DISTANCE = 10; 

constexpr float TERRAIN_BASE_HEIGHT = 20.0f;
constexpr float TERRAIN_AMPLITUDE = 6.0f;
constexpr float TERRAIN_FREQUENCY = 0.05f;
constexpr int STONE_LAYER_DEPTH = 4;