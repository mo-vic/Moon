#ifndef __SHADER_H_
#define __SHADER_H_

#include <glad/gl.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    // The Program ID
    unsigned int id;

    // Constructor reads and builds the shader
    Shader(const char * vertexPath, const char * fragmentPath);
    // Use/Active the Shader
    void use();
    // Utility uniform functions
    void setBool(const std::string & name, bool value) const;
    void setInt(const std::string & name, int value) const;
    void setFloat(const std::string & name, float value) const;
private:
    int check_compilation(unsigned int shader);
    int check_linking(unsigned int program);
};

Shader::Shader(const char * vertexPath, const char * fragmentPath)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Ensure the ifstream objects can throw exceptions 
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        // Open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        // Read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        // Close the file handlers
        vShaderFile.close();
        fShaderFile.close();

        // Convert stream into str
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch(const std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        std::cerr << e.what() << '\n';
    }

    const char * vShaderCode = vertexCode.c_str();
    const char * fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders
    unsigned int vertex;
    unsigned int fragment;
    int success;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    success = check_compilation(vertex);

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    success = check_compilation(fragment);

    // Shader program
    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    success = check_linking(id);

    // Delete the shader as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use()
{
    glUseProgram(id);
}

void Shader::setBool(const std::string & name, bool value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), (int) value);
}

void Shader::setInt(const std::string & name, int value) const
{
    glUniform1i(glGetUniformLocation(id, name.c_str()), value);
}

void Shader::setFloat(const std::string & name, float value) const
{
    glUniform1f(glGetUniformLocation(id, name.c_str()), value);
}

int Shader::check_compilation(unsigned int shader)
{
    int success;
    char infolog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infolog);
        std::cout << infolog << std::endl;
        exit(-1);
    }
    return success;
}

int Shader::check_linking(unsigned int program)
{
    int success;
    char infolog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infolog);
        std::cout << infolog << std::endl;
        exit(-1);
    }
    return success;
}

#endif
