#include "Shader.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace maze {
namespace {

GLuint compileStage(GLenum type, const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Shader: cannot open " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string source = ss.str();
    const char* src = source.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Shader: compile failed for " + path + ":\n" + log);
    }
    return shader;
}

std::string readFileOrEmpty(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

} // namespace

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    const std::string vertSrc = readFileOrEmpty(vertPath);
    const std::string fragSrc = readFileOrEmpty(fragPath);
    if (vertSrc.empty()) throw std::runtime_error("Shader: cannot read " + vertPath);
    if (fragSrc.empty()) throw std::runtime_error("Shader: cannot read " + fragPath);

    GLuint vert = compileStage(GL_VERTEX_SHADER, vertPath);
    GLuint frag = 0;
    try {
        frag = compileStage(GL_FRAGMENT_SHADER, fragPath);
    } catch (...) {
        glDeleteShader(vert);
        throw;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vert);
    glAttachShader(program_, frag);
    glLinkProgram(program_);
    glDeleteShader(vert); // flagged for deletion; freed on program delete
    glDeleteShader(frag);

    GLint ok = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        destroy();
        throw std::runtime_error("Shader: link failed (" + vertPath + ", " +
                                 fragPath + "):\n" + log);
    }
}

Shader::~Shader() { destroy(); }

Shader::Shader(Shader&& other) noexcept
    : program_(other.program_), uniformCache_(std::move(other.uniformCache_)) {
    other.program_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        destroy();
        program_ = other.program_;
        uniformCache_ = std::move(other.uniformCache_);
        other.program_ = 0;
    }
    return *this;
}

void Shader::destroy() {
    if (program_ != 0) glDeleteProgram(program_);
    program_ = 0;
}

void Shader::bind() const { glUseProgram(program_); }
void Shader::unbind() const { glUseProgram(0); }

GLint Shader::uniform(const char* name) const {
    const auto it = uniformCache_.find(name);
    if (it != uniformCache_.end()) return it->second;
    const GLint loc = glGetUniformLocation(program_, name);
    uniformCache_[name] = loc; // -1 (optimised out) is cached too, that is fine
    return loc;
}

void Shader::setInt(const char* name, GLint v) const {
    glUniform1i(uniform(name), v);
}
void Shader::setFloat(const char* name, float v) const {
    glUniform1f(uniform(name), v);
}
void Shader::setVec2(const char* name, float x, float y) const {
    glUniform2f(uniform(name), x, y);
}
void Shader::setVec3(const char* name, float x, float y, float z) const {
    glUniform3f(uniform(name), x, y, z);
}
void Shader::setVec3(const char* name, const float v[3]) const {
    glUniform3f(uniform(name), v[0], v[1], v[2]);
}
void Shader::setVec4(const char* name, float x, float y, float z, float w) const {
    glUniform4f(uniform(name), x, y, z, w);
}
void Shader::setMat4(const char* name, const mat4& m) const {
    glUniformMatrix4fv(uniform(name), 1, GL_FALSE, m.m);
}

} // namespace maze
