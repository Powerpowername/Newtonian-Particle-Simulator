#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class Shader
{
protected:
    unsigned int ShaderID;
    bool showCode = 0;
    void checkCompileErrors(GLuint shader, std::string type);
public:
    Shader(const char* vertexPath,const char* fragmentPath,const char* geometryPath = nullptr)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::fstream vShaderFile;
        std::fstream fShaderFile;
        std::fstream gShaderFile;

        vShaderFile.exceptions (std::fstream::failbit | std::fstream::badbit);
        fShaderFile.exceptions (std::fstream::failbit | std::fstream::badbit);
        gShaderFile.exceptions (std::fstream::failbit | std::fstream::badbit);
        
        try
        {
            //中转缓冲区，切换到string
            std::stringstream tempBuffer;
            //打开文件
            vShaderFile.open(vertexPath, std::ios::in);
            fShaderFile.open(fragmentPath,std::ios::in);
            //读取文件内容
            tempBuffer << vShaderFile.rdbuf();
            vertexCode = tempBuffer.str();
            tempBuffer.str("");
            tempBuffer.clear();//重置中转缓冲区中的状态标志（如错误或结束标志）
            
            tempBuffer << fShaderFile.rdbuf();
            fragmentCode = tempBuffer.str();
            tempBuffer.str("");
            tempBuffer.clear();

            if(geometryPath != nullptr)
            {
                gShaderFile.open(geometryPath,std::ios::in);
                tempBuffer << gShaderFile.rdbuf();
                geometryCode = tempBuffer.str();
                tempBuffer.str("");
                tempBuffer.clear();
            }
        }
        catch(std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        //vertexCode.c_str()返回的是右值
        const char* vShaderCodePoint = vertexCode.c_str();
        const char* fShaderCodePoint = fragmentCode.c_str();
        if(showCode)
        {
            std::cout << "" << vShaderCodePoint << std::endl;
            std::cout << fShaderCodePoint << std::endl;
        }
        //编译shader
        unsigned int vertex,fragment;
        //vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCodePoint, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        //fragment shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCodePoint, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        //geometry shader
        unsigned int geometry;
        if(geometryPath != nullptr)
        {
            const char * gShaderCodePoint = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCodePoint, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        //shader program,先关联再解绑，一个着色器程序可以关联多个着色器，但是最中链接的只有一个
        ShaderID = glCreateProgram();
        glAttachShader(ShaderID, vertex);
        glAttachShader(ShaderID, fragment);
        if(geometryPath != nullptr)
            glAttachShader(ShaderID, geometry);
        glLinkProgram(ShaderID);
        checkCompileErrors(ShaderID, "PROGRAM");
        //删除着色器只保留着色器程序
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if(geometryPath != nullptr)
            glDeleteShader(geometry);
    }

    unsigned int GetShaderID(); 
    // activate the shader
    // ------------------------------------------------------------------------
    void use() 
    { 
        glUseProgram(ShaderID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ShaderID, name.c_str()), (int)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ShaderID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ShaderID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ShaderID, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ShaderID, name.c_str()), x, y); 
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ShaderID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ShaderID, name.c_str()), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ShaderID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) 
    { 
        glUniform4f(glGetUniformLocation(ShaderID, name.c_str()), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ShaderID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ShaderID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ShaderID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
};