#pragma once

#include "raylib.h"

class Player {
public:
    Player();
    ~Player() = default;

    void Update(float deltaTime);
    void DrawCrosshair() const;

    Camera3D GetCamera() const { return camera; }
    Vector3 GetPosition() const { return position; }
    Vector3 GetForward() const;

    // Теперь камера обновляется мгновенно при смене позиции
    void SetPosition(Vector3 pos);

private:
    Vector3 position;
    Vector3 velocity;
    Camera3D camera;
    bool isGrounded = false;
    float yaw = 0.0f;
    float pitch = 0.0f;

    void HandleMouseLook();
    void HandleMovement(float deltaTime);
    void ApplyPhysics(float deltaTime);
    void UpdateCamera();
};