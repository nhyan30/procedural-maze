// Shader.hpp - tiny RAII wrapper around an OpenGL shader program.
//
// Handles compiling vertex+fragment files, linking, uniform location caching
// and a handful of typed setters. On failure it throws with the driver log so
// the message reaches the console.
#pragma once

#include <string>
#include <unordered_map>

#include <glad/gl.h>

#include "Math.hpp"

namespace maze {

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void bind() const;
    void unbind() const;

    // Locations are looked up once and cached; repeated per-frame queries
    // therefore cost a hash lookup instead of a GL introspection call.
    GLint uniform(const char* name) const;

    void setInt(const char* name, GLint v) const;
    void setFloat(const char* name, float v) const;
    void setVec2(const char* name, float x, float y) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setVec3(const char* name, const float v[3]) const;
    void setVec4(const char* name, float x, float y, float z, float w) const;
    void setMat4(const char* name, const mat4& m) const;

private:
    void destroy();

    GLuint program_ = 0;
    mutable std::unordered_map<std::string, GLint> uniformCache_;
};

} // namespace maze
