#pragma once
#include <vector>
#include "thirdparty_header.h"

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

const float YAW = 0.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.05f;
const float ZOOM = 90.0f;


class GLCamera {
public:
    GLCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 front = glm::vec3(0.f, 0.f, 1.f), 
        float yaw = YAW, float pitch = PITCH) 
        {
            Position = position;
            WorldUp = up;
            Yaw = yaw;
            Pitch = pitch;
            Front = front;
            updateCameraVectors();
        }
    ~GLCamera(){}

    glm::mat4 GetViewMatrix() 
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD) 
        {
            Position += Front * velocity;
        }
        
        if (direction == BACKWARD) 
        {
            Position -= Front * velocity;
        }
        
        if (direction == LEFT) 
        {
            Position += Right * velocity;
        }
        
        if (direction == RIGHT) 
        {
            Position -= Right * velocity;
        }
        if (direction == UP)
        {
            Position += WorldUp * velocity;
        }

        if (direction == DOWN)
        {
            Position -= WorldUp * velocity;
        }

        printf("After key drived move, velocity: %F: position: (%f, %f, %f), MovementSpeed: %f, deltaTime: %f\n", 
            velocity, Position.x, Position.y, Position.z, MovementSpeed, deltaTime);
        
        updateCameraVectors();
    }

    void ProcessMoveMovement(float xoffset, float yoffset, bool constrainPitch = true) 
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch) 
        {
            Pitch = std::max(-89.0f, std::min(89.0f, Pitch));
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) 
    {
        if (Zoom >= 1.0f && Zoom <= 45.0f) 
        {
            Zoom -= yoffset;
        }

        if (Zoom <= 1.0f) 
        {
            Zoom = 1.0f;
        }

        if (Zoom >= 45.0f) 
        {
            Zoom = 45.0f;
        }
    }

    void updateCameraVectors() 
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
        front.y = sin(glm::radians(Pitch));
        front.z = cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(WorldUp, Front));
        Up = glm::normalize(glm::cross(Right, Front));
    }

public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed = 1.f;
    float MouseSensitivity = 0.02f;
    float Zoom = 45.0f;
};
