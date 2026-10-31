#include "player.h"
#include "config.h"
#include "world.h"
#include "raymath.h"
#include <cmath>

Player::Player() {
    position = { 0.0f, 50.0f, 0.0f };
    velocity = { 0, 0, 0 };
    camera.position = position;
    camera.target = Vector3Add(position, Vector3{0, 0, -1});
    camera.up = { 0, 1, 0 };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    yaw = -90.0f;
    pitch = 0.0f;
    UpdateCamera();
}

void Player::SetPosition(Vector3 pos) { position = pos; UpdateCamera(); }
void Player::Update(float deltaTime) { HandleMouseLook(); HandleMovement(deltaTime); ApplyPhysics(deltaTime); UpdateCamera(); }

void Player::HandleMouseLook() {
    Vector2 mouseDelta = GetMouseDelta();
    yaw += mouseDelta.x * PLAYER_MOUSE_SENSITIVITY * 100.0f;
    
    // ⬇️ ФИКС: Используем fmodf для нормализации ⬇️
    yaw = fmodf(yaw, 360.0f);
    if (yaw < 0.0f) yaw += 360.0f;

    pitch -= mouseDelta.y * PLAYER_MOUSE_SENSITIVITY * 100.0f;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void Player::HandleMovement(float deltaTime) {
    Vector3 forward = { cosf(DEG2RAD * yaw), 0.0f, sinf(DEG2RAD * yaw) };
    forward = Vector3Normalize(forward);
    Vector3 right = { -forward.z, 0.0f, forward.x };

    Vector3 move = {0, 0, 0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    move = Vector3Add(move, forward);
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  move = Vector3Subtract(move, forward);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move = Vector3Add(move, right);
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  move = Vector3Subtract(move, right);

    if (Vector3Length(move) > 0.001f) {
        move = Vector3Normalize(move);
        velocity.x = move.x * PLAYER_MOVE_SPEED;
        velocity.z = move.z * PLAYER_MOVE_SPEED;
    } else {
        velocity.x = 0; velocity.z = 0;
    }

    if (IsKeyPressed(KEY_SPACE) && isGrounded) {
        velocity.y = PLAYER_JUMP_FORCE;
        isGrounded = false;
    }
}

void Player::ApplyPhysics(float deltaTime) {
    velocity.y += PLAYER_GRAVITY * deltaTime;
    Vector3 newPos = Vector3Add(position, Vector3Scale(velocity, deltaTime));

    auto collides = [&](Vector3 p) -> bool {
        int minX = static_cast<int>(std::floor(p.x - PLAYER_RADIUS));
        int maxX = static_cast<int>(std::floor(p.x + PLAYER_RADIUS));
        int minY = static_cast<int>(std::floor(p.y));
        int maxY = static_cast<int>(std::floor(p.y + PLAYER_HEIGHT - 0.01f));
        int minZ = static_cast<int>(std::floor(p.z - PLAYER_RADIUS));
        int maxZ = static_cast<int>(std::floor(p.z + PLAYER_RADIUS));

        for (int x = minX; x <= maxX; x++)
            for (int y = minY; y <= maxY; y++)
                for (int z = minZ; z <= maxZ; z++)
                    if (World::GetBlock(x, y, z) != BlockType::Air) return true;
        return false;
    };

    Vector3 testX = { newPos.x, position.y, position.z };
    if (!collides(testX)) position.x = testX.x;
    Vector3 testZ = { position.x, position.y, newPos.z };
    if (!collides(testZ)) position.z = testZ.z;
    Vector3 testY = { position.x, newPos.y, position.z };
    if (!collides(testY)) {
        position.y = testY.y;
        isGrounded = false;
    } else {
        if (velocity.y < 0) isGrounded = true;
        velocity.y = 0;
    }
}

void Player::UpdateCamera() {
    Vector3 forward = {
        cosf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch),
        sinf(DEG2RAD * pitch),
        sinf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch)
    };
    camera.position = { position.x, position.y + PLAYER_HEIGHT * 0.9f, position.z };
    camera.target = Vector3Add(camera.position, Vector3Normalize(forward));
}

Vector3 Player::GetForward() const {
    return {
        cosf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch),
        sinf(DEG2RAD * pitch),
        sinf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch)
    };
}

void Player::DrawCrosshair() const {
    int cx = GetScreenWidth() / 2;
    int cy = GetScreenHeight() / 2;
    DrawLine(cx - 10, cy, cx + 10, cy, WHITE);
    DrawLine(cx, cy - 10, cx, cy + 10, WHITE);
}