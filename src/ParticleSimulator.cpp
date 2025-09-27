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

// 定义与GLSL着色器中完全匹配的数据结构，确保CPU和GPU内存布局一致
struct PackedVector3 {
    float X, Y, Z; // 与GLSL中的PackedVector3结构体布局一致
};

// 粒子数据结构，包含位置和速度信息
struct Particle {
    PackedVector3 Position; // 粒子当前位置
    PackedVector3 Velocity; // 粒子当前速度
};

// 粒子模拟器主类
class ParticleSimulator {
private:
    unsigned int numParticles;    // 粒子总数，在构造时初始化后不再改变
    unsigned int particleSSBO;    // 着色器存储缓冲区对象(SSBO)的OpenGL标识符
    unsigned int shaderProgram;   // 着色器程序的OpenGL标识符
    bool isRunning;               // 模拟运行状态标志

    // 从文件加载着色器源代码的辅助函数
    std::string loadShaderSource(const char* filePath) {
        std::string content; // 存储文件内容的字符串
        std::ifstream fileStream(filePath, std::ios::in); // 创建输入文件流
        if (!fileStream.is_open()) { // 检查文件是否成功打开
            // 实际应用中应该记录错误或抛出异常
            return ""; // 返回空字符串表示加载失败
        }
        std::string line; // 用于逐行读取的临时字符串
        while (!fileStream.eof()) { // 循环直到文件结束
            std::getline(fileStream, line); // 读取一行内容
            content.append(line + "\n"); // 将行内容添加到结果字符串，并添加换行符
        }
        fileStream.close(); // 关闭文件流
        return content; // 返回文件内容
    }

    // 编译着色器的辅助函数
    unsigned int compileShader(GLenum type, const char* source) {
        unsigned int id = glCreateShader(type); // 创建指定类型的着色器对象
        glShaderSource(id, 1, &source, nullptr); // 将源代码关联到着色器对象
        glCompileShader(id); // 编译着色器
        
        // 检查编译是否成功
        int success;
        char infoLog[512]; // 存储错误信息的缓冲区
        glGetShaderiv(id, GL_COMPILE_STATUS, &success); // 获取编译状态
        if (!success) { // 如果编译失败
            glGetShaderInfoLog(id, 512, nullptr, infoLog); // 获取错误信息
            // 实际应用中应该记录错误或抛出异常
        }
        
        return id; // 返回着色器对象的OpenGL标识符
    }

public:
    // 构造函数：使用初始粒子数据初始化模拟器
    ParticleSimulator(const std::vector<Particle>& particles) : numParticles(particles.size()) {
        // 加载顶点着色器源代码
        std::string vertSource = loadShaderSource("res/shaders/particles/vertex.glsl");
        // 加载片段着色器源代码
        std::string fragSource = loadShaderSource("res/shaders/particles/fragment.glsl");
        
        // 编译顶点着色器
        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertSource.c_str());
        // 编译片段着色器
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragSource.c_str());
        
        // 创建着色器程序
        shaderProgram = glCreateProgram();
        // 附加顶点着色器到程序
        glAttachShader(shaderProgram, vertexShader);
        // 附加片段着色器到程序
        glAttachShader(shaderProgram, fragmentShader);
        // 链接着色器程序
        glLinkProgram(shaderProgram);
        
        // 检查链接是否成功
        int success;
        char infoLog[512];
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) { // 如果链接失败
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            // 实际应用中应该记录错误或抛出异常
        }
        
        // 删除着色器对象，它们已经链接到程序中，不再需要单独存在
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // 创建着色器存储缓冲区对象(SSBO)
        glGenBuffers(1, &particleSSBO); // 生成缓冲区对象
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO); // 绑定到SSBO目标
        // 分配GPU内存并初始化数据
        glBufferData(GL_SHADER_STORAGE_BUFFER, // 目标类型
                    numParticles * sizeof(Particle), // 缓冲区大小（字节）
                    particles.data(), // 初始化数据指针
                    GL_DYNAMIC_DRAW); // 使用方式：动态绘制（CPU可能会更新）
        // 将SSBO绑定到绑定点0，与着色器中的binding = 0对应
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleSSBO);
        
        // 初始化模拟状态为运行中
        isRunning = true;
        setRunning(isRunning); // 设置对应的uniform值
    }

    // 析构函数：清理所有OpenGL资源
    ~ParticleSimulator() {
        glDeleteBuffers(1, &particleSSBO); // 删除SSBO
        glDeleteProgram(shaderProgram); // 删除着色器程序
    }

    // 设置模拟运行状态
    void setRunning(bool running) {
        isRunning = running; // 更新内部状态
        glUseProgram(shaderProgram); // 激活着色器程序
        // 获取uniform变量"isRunning"的位置
        GLint location = glGetUniformLocation(shaderProgram, "isRunning");
        // 设置uniform值：1.0f表示运行，0.0f表示停止
        glUniform1f(location, isRunning ? 1.0f : 0.0f);
    }

    // 运行一帧模拟
    void run(float dT) {
        glPointSize(1.1f); // 设置点精灵的大小（像素）
        glEnable(GL_BLEND); // 启用颜色混合
        glBlendEquation(GL_FUNC_ADD); // 设置混合方程：源颜色 + 目标颜色
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 设置混合函数：源alpha，1-源alpha

        glClear(GL_COLOR_BUFFER_BIT); // 清除颜色缓冲区
        glUseProgram(shaderProgram); // 使用粒子着色器程序
        
        // 获取时间增量uniform的位置并设置值
        GLint dTLocation = glGetUniformLocation(shaderProgram, "dT");
        glUniform1f(dTLocation, dT); // 上传时间增量

        // 绘制点精灵 - 每个粒子对应一个点
        glDrawArrays(GL_POINTS, // 图元类型：点
                    0, // 起始索引
                    numParticles); // 顶点数量
        
        // 内存屏障：确保着色器对SSBO的写入在后续操作中可见
        // 防止GPU乱序执行导致的数据竞争
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // 处理输入事件
    void processInputs(GLFWwindow* window, const glm::vec3& camPos, const glm::mat4& view, const glm::mat4& projection) {
        // 检查鼠标输入（仅当光标可见时）
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                double xpos, ypos;
                // 获取鼠标光标位置（窗口坐标）
                glfwGetCursorPos(window, &xpos, &ypos);
                int width, height;
                // 获取窗口尺寸
                glfwGetWindowSize(window, &width, &height);
                ypos = height - ypos; // 反转Y轴（OpenGL坐标系与窗口坐标系不同）

                // 将窗口坐标转换为标准化设备坐标（NDC）[-1, 1]范围
                glm::vec2 normalizedDeviceCoords = glm::vec2(
                    (2.0f * xpos) / width - 1.0f, // X坐标转换
                    (2.0f * ypos) / height - 1.0f  // Y坐标转换
                );

                // 计算逆投影矩阵和逆视图矩阵
                glm::mat4 invProjection = glm::inverse(projection);
                glm::mat4 invView = glm::inverse(view);
                // 计算从相机位置发出的世界空间射线方向
                glm::vec3 dir = getWorldSpaceRay(invProjection, invView, normalizedDeviceCoords);

                // 计算质心点位置（沿射线方向25单位距离处）
                glm::vec3 pointOfMass = camPos + dir * 25.0f;
                
                // 上传质心点到着色器
                GLint massLocation = glGetUniformLocation(shaderProgram, "pointOfMass");
                glUniform3fv(massLocation, 1, glm::value_ptr(pointOfMass));
                
                // 激活吸引力
                GLint attractLocation = glGetUniformLocation(shaderProgram, "isActive");
                glUniform1f(attractLocation, 1.0f);
            } else {
                // 禁用吸引力
                GLint attractLocation = glGetUniformLocation(shaderProgram, "isActive");
                glUniform1f(attractLocation, 0.0f);
            }
        }

        // 处理键盘输入：T键切换模拟运行状态
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            static bool keyPressed = false; // 静态变量跟踪按键状态
            if (!keyPressed) { // 只在按键刚按下时触发
                setRunning(!isRunning); // 切换运行状态
                keyPressed = true; // 标记按键已处理
            }
        } else {
            keyPressed = false; // 按键释放时重置状态
        }

        // 计算并上传视图-投影矩阵
        glm::mat4 viewProjection = projection * view;
        GLint vpLocation = glGetUniformLocation(shaderProgram, "projViewMatrix");
        glUniformMatrix4fv(vpLocation, // uniform位置
                          1, // 矩阵数量
                          GL_FALSE, // 是否转置（GLM默认列主序，OpenGL需要列主序）
                          glm::value_ptr(viewProjection)); // 矩阵数据指针
    }

    // 静态辅助函数：计算从相机位置发出的世界空间射线方向
    static glm::vec3 getWorldSpaceRay(const glm::mat4& inverseProjection, const glm::mat4& inverseView, const glm::vec2& normalizedDeviceCoords) {
        // 将NDC坐标转换到眼空间
        glm::vec4 rayEye = inverseProjection * glm::vec4(normalizedDeviceCoords, -1.0f, 1.0f);//此处-1为近平面的意思
        rayEye.z = -1.0f; // 不设置最简单，矩阵浮点数运算可能有误差，所以将z设为-1，表示射线从近平面发出，和前面要求是近平面一致
        rayEye.w = 0.0f;  // 设置w分量为0（表示方向而非位置）
        // 将眼空间射线转换到世界空间
        glm::vec4 rayWorld = inverseView * rayEye;
        // 返回归一化的方向向量
        return glm::normalize(glm::vec3(rayWorld));
    }
};