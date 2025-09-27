#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "particle_simulator.h"
#include "helper.h"

class MainWindow {
private:
    GLFWwindow* window;
    int width = 832;
    int height = 832;
    std::string title = "Newtonian-Particle-Simulator";
    
    Camera camera;
    glm::mat4 projection;
    
    int frames = 0;
    int FPS = 0;
    std::chrono::high_resolution_clock::time_point fpsStartTime;
    
    ParticleSimulator* particleSimulator;
    bool cursorVisible = true;
    bool cursorGrabbed = false;
    bool vsync = false;

public:
    MainWindow() : camera(glm::vec3(0, 0, 15), glm::vec3(0, 1, 0)) {
        initWindow();
        fpsStartTime = std::chrono::high_resolution_clock::now();
    }
    
    ~MainWindow() {
        delete particleSimulator;
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    void run() {
        while (!glfwWindowShouldClose(window)) {
            double currentTime = glfwGetTime();
            static double lastTime = currentTime;
            float deltaTime = static_cast<float>(currentTime - lastTime);
            lastTime = currentTime;
            
            updateFPS();
            processInput(deltaTime);
            render(deltaTime);
            
            glfwPollEvents();
        }
    }

private:
    void initWindow() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        
        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // 打印OpenGL信息
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
        std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;
        
        // 检查扩展支持
        if (!Helper::isCoreExtensionAvailable("GL_ARB_direct_state_access", 4, 5)) {
            throw std::runtime_error("Your system does not support GL_ARB_direct_state_access");
        }
        
        if (!Helper::isCoreExtensionAvailable("GL_ARB_buffer_storage", 4, 4)) {
            throw std::runtime_error("Your system does not support GL_ARB_buffer_storage");
        }
        
        // 设置垂直同步
        glfwSwapInterval(0); // 关闭VSync
        
        // 初始化粒子
        int numParticles;
        do {
            std::cout << "Number of particles: ";
            std::cin >> numParticles;
        } while (numParticles < 0);
        
        std::vector<Particle> particles(numParticles);
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<float> dist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> zDist(-100.0f, 0.0f);
        
        for (int i = 0; i < numParticles; i++) {
            particles[i].position = glm::vec3(dist(rng), dist(rng), zDist(rng));
            // 可选: 使用随机单位向量初始化
            // particles[i].position = Helper::randomUnitVector(rng) * 50.0f;
        }
        
        particleSimulator = new ParticleSimulator(particles);
        
        // 初始化投影矩阵
        projection = glm::perspective(glm::radians(103.0f), (float)width / height, 0.1f, 1000.0f);
    }
    
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        auto* mainWindow = static_cast<MainWindow*>(glfwGetWindowUserPointer(window));
        if (mainWindow) {
            mainWindow->width = width;
            mainWindow->height = height;
            glViewport(0, 0, width, height);
            mainWindow->projection = glm::perspective(
                glm::radians(103.0f), 
                (float)width / height, 
                0.1f, 
                1000.0f
            );
        }
    }
    
    void updateFPS() {
        frames++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - fpsStartTime).count();
        
        if (elapsed >= 1000) {
            FPS = frames;
            std::string newTitle = title + " FPS: " + std::to_string(FPS);
            glfwSetWindowTitle(window, newTitle.c_str());
            frames = 0;
            fpsStartTime = currentTime;
        }
    }
    
    void processInput(float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        
        // 切换垂直同步
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            vsync = !vsync;
            glfwSwapInterval(vsync ? 1 : 0);
            // 短暂延迟防止快速切换
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // 切换鼠标状态
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            cursorVisible = !cursorVisible;
            cursorGrabbed = !cursorGrabbed;
            
            glfwSetInputMode(window, GLFW_CURSOR, cursorVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            
            if (!cursorVisible) {
                camera.setVelocity(glm::vec3(0));
            }
            
            // 短暂延迟防止快速切换
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // 切换全屏
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
            static bool isFullscreen = false;
            static int windowedX, windowedY, windowedWidth, windowedHeight;
            
            if (isFullscreen) {
                glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
            } else {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                
                glfwGetWindowPos(window, &windowedX, &windowedY);
                glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
                
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            
            isFullscreen = !isFullscreen;
            // 短暂延迟防止快速切换
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // 处理相机输入
        if (!cursorVisible) {
            camera.processInputs(window, deltaTime);
        }
        
        // 处理粒子模拟器输入
        particleSimulator->processInputs(camera.getPosition(), camera.getViewMatrix(), projection);
    }
    
    void render(float deltaTime) {
        particleSimulator->run(deltaTime);
        glfwSwapBuffers(window);
    }
};

int main() {
    try {
        MainWindow window;
        window.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}