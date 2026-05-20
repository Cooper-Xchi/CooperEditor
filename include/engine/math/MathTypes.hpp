#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace engine::math {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Mat4 {
    std::array<float, 16> elements{};

    static Mat4 identity() {
        Mat4 matrix;
        matrix.elements = {1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f};
        return matrix;
    }
};

inline Vec3 operator+(const Vec3& left, const Vec3& right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

inline Vec3 operator-(const Vec3& left, const Vec3& right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

inline Vec3 operator*(const Vec3& vector, float scalar) {
    return {vector.x * scalar, vector.y * scalar, vector.z * scalar};
}

inline float dot(const Vec3& left, const Vec3& right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

inline Vec3 cross(const Vec3& left, const Vec3& right) {
    return {left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x};
}

inline float length(const Vec3& vector) {
    return std::sqrt(dot(vector, vector));
}

inline Vec3 normalize(const Vec3& vector) {
    const float vectorLength = length(vector);
    if (vectorLength <= 0.0f) {
        return {};
    }
    return vector * (1.0f / vectorLength);
}

inline float radians(float degrees) {
    return degrees * 0.01745329251994329577f;
}

inline float clamp(float value, float minValue, float maxValue) {
    return std::clamp(value, minValue, maxValue);
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 forward = normalize(center - eye);
    const Vec3 side = normalize(cross(forward, up));
    const Vec3 trueUp = cross(side, forward);

    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = side.x;
    matrix.elements[4] = side.y;
    matrix.elements[8] = side.z;
    matrix.elements[1] = trueUp.x;
    matrix.elements[5] = trueUp.y;
    matrix.elements[9] = trueUp.z;
    matrix.elements[2] = -forward.x;
    matrix.elements[6] = -forward.y;
    matrix.elements[10] = -forward.z;
    matrix.elements[12] = -dot(side, eye);
    matrix.elements[13] = -dot(trueUp, eye);
    matrix.elements[14] = dot(forward, eye);
    return matrix;
}

inline Mat4 perspective(float verticalFovRadians,
                        float aspectRatio,
                        float nearClip,
                        float farClip) {
    const float tanHalfFov = std::tan(verticalFovRadians * 0.5f);

    Mat4 matrix{};
    matrix.elements[0] = 1.0f / (aspectRatio * tanHalfFov);
    matrix.elements[5] = 1.0f / tanHalfFov;
    matrix.elements[10] = farClip / (nearClip - farClip);
    matrix.elements[11] = -1.0f;
    matrix.elements[14] = -(farClip * nearClip) / (farClip - nearClip);
    return matrix;
}

inline Mat4 orthographic(float left,
                         float right,
                         float bottom,
                         float top,
                         float nearClip,
                         float farClip) {
    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = 2.0f / (right - left);
    matrix.elements[5] = 2.0f / (top - bottom);
    matrix.elements[10] = 1.0f / (nearClip - farClip);
    matrix.elements[12] = -(right + left) / (right - left);
    matrix.elements[13] = -(top + bottom) / (top - bottom);
    matrix.elements[14] = nearClip / (nearClip - farClip);
    return matrix;
}

inline Mat4 multiply(const Mat4& left, const Mat4& right) {
    Mat4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0f;
            for (int inner = 0; inner < 4; ++inner) {
                value += left.elements[inner * 4 + row] *
                         right.elements[column * 4 + inner];
            }
            result.elements[column * 4 + row] = value;
        }
    }
    return result;
}

inline Mat4 translation(float x, float y, float z) {
    Mat4 matrix = Mat4::identity();
    matrix.elements[12] = x;
    matrix.elements[13] = y;
    matrix.elements[14] = z;
    return matrix;
}

inline Mat4 scaleMatrix(float x, float y, float z) {
    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = x;
    matrix.elements[5] = y;
    matrix.elements[10] = z;
    return matrix;
}

inline Mat4 rotationX(float radiansValue) {
    Mat4 matrix = Mat4::identity();
    const float cosine = std::cos(radiansValue);
    const float sine = std::sin(radiansValue);
    matrix.elements[5] = cosine;
    matrix.elements[6] = sine;
    matrix.elements[9] = -sine;
    matrix.elements[10] = cosine;
    return matrix;
}

inline Mat4 rotationY(float radiansValue) {
    Mat4 matrix = Mat4::identity();
    const float cosine = std::cos(radiansValue);
    const float sine = std::sin(radiansValue);
    matrix.elements[0] = cosine;
    matrix.elements[2] = -sine;
    matrix.elements[8] = sine;
    matrix.elements[10] = cosine;
    return matrix;
}

inline Mat4 rotationZ(float radiansValue) {
    Mat4 matrix = Mat4::identity();
    const float cosine = std::cos(radiansValue);
    const float sine = std::sin(radiansValue);
    matrix.elements[0] = cosine;
    matrix.elements[1] = sine;
    matrix.elements[4] = -sine;
    matrix.elements[5] = cosine;
    return matrix;
}

}  // namespace engine::math
