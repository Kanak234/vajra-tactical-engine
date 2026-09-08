#include "Renderer/Shader.h"

#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

#include "Core/Log.h"

namespace vajra {

std::string Shader::readFile(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        VJ_ERROR("Shader file not found: %s", path.c_str());
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum stage, const std::string& source, const std::string& label) {
    if (source.empty()) return 0;
    const GLuint sh = glCreateShader(stage);
    const char* src = source.c_str();
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len) + 1, '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        VJ_ERROR("Compile failed [%s]:\n%s", label.c_str(), log.data());
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

Shader::Shader(std::string vertPath, std::string fragPath)
    : m_vertPath(std::move(vertPath)), m_fragPath(std::move(fragPath)) {
    reload();
}

bool Shader::reload() {
    const GLuint vs = compile(GL_VERTEX_SHADER,   readFile(m_vertPath), m_vertPath);
    const GLuint fs = compile(GL_FRAGMENT_SHADER, readFile(m_fragPath), m_fragPath);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;   // keep the previous working program alive
    }

    const GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len) + 1, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        VJ_ERROR("Link failed [%s]:\n%s", m_vertPath.c_str(), log.data());
        glDeleteProgram(prog);
        return false;
    }

    if (m_id) glDeleteProgram(m_id);
    m_id = prog;
    m_uniforms.clear();
    return true;
}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

Shader::Shader(Shader&& other) noexcept
    : m_id(other.m_id),
      m_vertPath(std::move(other.m_vertPath)),
      m_fragPath(std::move(other.m_fragPath)),
      m_uniforms(std::move(other.m_uniforms)) {
    other.m_id = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteProgram(m_id);
        m_id       = other.m_id;
        m_vertPath = std::move(other.m_vertPath);
        m_fragPath = std::move(other.m_fragPath);
        m_uniforms = std::move(other.m_uniforms);
        other.m_id = 0;
    }
    return *this;
}

GLint Shader::loc(const char* name) {
    if (const auto it = m_uniforms.find(name); it != m_uniforms.end())
        return it->second;
    const GLint l = glGetUniformLocation(m_id, name);
    m_uniforms.emplace(name, l);
    return l;
}

}  // namespace vajra
