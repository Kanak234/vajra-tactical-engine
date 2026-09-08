#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace vajra {

/// GLSL program with hot-reload support (F5 in dev builds) and cached
/// uniform locations so per-frame lookups are a hash hit, not a driver call.
class Shader {
public:
    Shader() = default;
    Shader(std::string vertPath, std::string fragPath);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void bind() const { glUseProgram(m_id); }
    bool reload();

    void set(const char* name, int v)               { glUniform1i(loc(name), v); }
    void set(const char* name, float v)             { glUniform1f(loc(name), v); }
    void set(const char* name, const glm::vec2& v)  { glUniform2fv(loc(name), 1, &v[0]); }
    void set(const char* name, const glm::vec3& v)  { glUniform3fv(loc(name), 1, &v[0]); }
    void set(const char* name, const glm::vec4& v)  { glUniform4fv(loc(name), 1, &v[0]); }
    void set(const char* name, const glm::mat4& v)  {
        glUniformMatrix4fv(loc(name), 1, GL_FALSE, &v[0][0]);
    }

    [[nodiscard]] GLuint id() const { return m_id; }
    [[nodiscard]] bool valid() const { return m_id != 0; }

private:
    GLint loc(const char* name);
    static GLuint compile(GLenum stage, const std::string& source, const std::string& label);
    static std::string readFile(const std::string& path);

    GLuint m_id = 0;
    std::string m_vertPath;
    std::string m_fragPath;
    std::unordered_map<std::string, GLint> m_uniforms;
};

}  // namespace vajra
