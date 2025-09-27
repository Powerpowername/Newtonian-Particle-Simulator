#version 430 core
#define EPSILON 0.001
const float DRAG_COEF = log(0.998) * 176.0; // 预计算的阻力系数

// 数据结构定义
struct PackedVector3
{
    float X, Y, Z;
};

struct Particle
{
    PackedVector3 Position;
    PackedVector3 Velocity;
};

// 着色器存储缓冲区(SSBO) - 用于存储和更新粒子数据
layout(std430, binding = 0) restrict buffer ParticlesSSBO
{
    Particle Particles[];
} particleSSBO;

// 统一变量(从CPU传递到GPU的控制参数)
layout(location = 0) uniform float dT;              // 时间步长
layout(location = 1) uniform vec3 pointOfMass;      // 质心位置
layout(location = 2) uniform float isActive;        // 模拟是否激活
layout(location = 3) uniform float isRunning;        // 模拟是否运行
layout(location = 4) uniform mat4 projViewMatrix;   // 投影视图矩阵

// 输出变量
out InOutVars
{
    vec3 Color;  // 基于速度计算的颜色
} outData;

// 工具函数：打包向量与GLSL原生向量间的转换
vec3 PackedVec3ToVec3(PackedVector3 vec)
{
    return vec3(vec.X, vec.Y, vec.Z);
}

PackedVector3 Vec3ToPackedVec3(vec3 vec)
{
    return PackedVector3(vec.x, vec.y, vec.z);
}

// 主函数
void main()
{
    // 1. 读取当前粒子数据
    PackedVector3 packedPosition = particleSSBO.Particles[gl_VertexID].Position;
    PackedVector3 packedVelocity = particleSSBO.Particles[gl_VertexID].Velocity;
    vec3 position = PackedVec3ToVec3(packedPosition);
    vec3 velocity = PackedVec3ToVec3(packedVelocity);

    // 2. 计算指向质心的向量
    const vec3 toMass = pointOfMass - position;
    
    // 3. 实现牛顿万有引力定律
    const float m1 = 1.0;   // 粒子质量(常量)
    const float m2 = 176.0; // 质心质量(用户可控制)
    const float G = 1.0;    // 引力常数(简化)
    const float m1_m2 = m1 * m2; // 质量乘积
    const float rSquared = max(dot(toMass, toMass), EPSILON * EPSILON); // 距离平方(防止除零)
    const vec3 force = toMass * (G * ((m1_m2) / rSquared)); // 引力计算
    
    // 4. 计算加速度(牛顿第二定律 F=ma)
    const vec3 acceleration = (force * isRunning * isActive) / m1;

    // 5. 应用阻力(指数衰减模型)
    velocity *= mix(1.0, exp(DRAG_COEF * dT), isRunning);
    
    // 6. 更新位置和速度(半隐式欧拉积分法)
    position += (dT * velocity + 0.5 * acceleration * dT * dT) * isRunning;
    velocity += acceleration * dT;

    // 7. 将更新后的数据写回SSBO
    particleSSBO.Particles[gl_VertexID].Position = Vec3ToPackedVec3(position);
    particleSSBO.Particles[gl_VertexID].Velocity = Vec3ToPackedVec3(velocity);

    // 8. 基于速度计算颜色
    const float red = 0.0045 * dot(velocity, velocity);          // 红色与速度平方成正比
    const float green = clamp(0.08 * max(velocity.x, max(velocity.y, velocity.z)), 0.2, 0.5); // 绿色与最大速度分量相关
    const float blue = 0.7 - red;                                // 蓝色与红色互补
    outData.Color = vec3(red, green, blue);

    // 9. 计算最终顶点位置
    gl_Position = projViewMatrix * vec4(position, 1.0);
}