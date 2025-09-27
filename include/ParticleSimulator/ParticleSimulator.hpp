#pragma once // 头文件保护宏，确保该头文件在同一个编译单元中只被包含一次

// 引入标准库组件
#include <vector>    // 使用std::vector存储粒子数据
#include <string>    // 使用std::string处理字符串
#include <fstream>   // 使用std::ifstream读取文件
#include <glad/glad.h> // OpenGL函数加载器，提供所有OpenGL函数的声明
#include <GLFW/glfw3.h> // 跨平台窗口和输入管理库
#include <glm/glm.hpp> // OpenGL数学库，提供向量、矩阵等数学工具
#include <glm/gtc/matrix_transform.hpp> // GLM的矩阵变换函数
#include <glm/gtc/type_ptr.hpp> // GLM的类型指针转换函数
using namespace glm;
struct Particle
{
    vec3 Position; // 粒子当前位置
    vec3 Velocity; // 粒子当前速度
};
// 粒子模拟器
class ParticleSimulator
{
protected:
    unsigned int numParticles; 
    unsigned int particleSSBO;
    unsigned int shaderProgram;
    bool isRunning;               // 模拟运行状态标志

public:
    void setShader(unsigned int shaderProgram);
    ParticleSimulator() = default;
    ~ParticleSimulator() = default;
    void init(const std::vector<Particle>& particles);

    void setRunning(bool running);    // 设置模拟运行状态
    void Render(float dT);
    

};

inline void ParticleSimulator::setShader(unsigned int shaderProgram)
{
    this->shaderProgram = shaderProgram;
}

inline void ParticleSimulator::init(const std::vector<Particle> &particles)
{
    numParticles = particles.size();
    glGenBuffers(1, &particleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);

    glBufferData(GL_SHADER_STORAGE_BUFFER,
                numParticles * sizeof(Particle),
                particles.data(),
                GL_DYNAMIC_DRAW);//后期考虑到底需不需要动态更新

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
    isRunning = true;
    setRunning(isRunning); // 设置对应的uniform值

}

inline ParticleSimulator::~ParticleSimulator()
{
    glDeleteBuffers(1, &particleSSBO);
    // glDeleteProgram(shaderProgram);
}

inline void ParticleSimulator::setRunning(bool running)
{
        isRunning = running; // 更新内部状态
        glUseProgram(shaderProgram); // 激活着色器程序
        // 获取uniform变量"isRunning"的位置
        GLint location = glGetUniformLocation(shaderProgram, "isRunning");
        // 设置uniform值：1.0f表示运行，0.0f表示停止
        glUniform1f(location, isRunning ? 1.0f : 0.0f);
}

inline void ParticleSimulator::Render(float dT)
{
    glPointSize(1.1f); // 设置点精灵的大小（像素）
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);                      // 设置混合方程：源颜色 + 目标颜色
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 设置混合函数：源alpha，1-源alpha


    glClear(GL_COLOR_BUFFER_BIT); // 清除颜色缓冲区
    glUseProgram(shaderProgram);  // 使用粒子着色器程序

    // 获取时间增量uniform的位置并设置值
    GLint dTLocation = glGetUniformLocation(shaderProgram, "dT");
    glUniform1f(dTLocation, dT); // 上传时间增量

    // 绘制点精灵 - 每个粒子对应一个点
    glDrawArrays(GL_POINTS,     // 图元类型：点
                 0,             // 起始索引
                 numParticles); // 顶点数量

    // 内存屏障：确保着色器对SSBO的写入在后续操作中可见
    // 防止GPU乱序执行导致的数据竞争
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
