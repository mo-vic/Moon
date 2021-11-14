#include <glad/gl.h>
#define GLFW_INCLUDE_NONE  // 避免头文件冲突
#include <GLFW/glfw3.h>


// C++ 标准库
#include <iostream>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>

// 提供向量和矩阵容器，以及相关的函数
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// 提供图像加载和保存的函数
#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb_image_write.h"

// 自定义的Shader类和Sphere类
#include "shaders/shader.h"
#include "geometry/sphere.hpp"

using namespace std;

// 处理键盘事件的函数
void process_input(GLFWwindow *);
// 窗口大小发生变化时的调用的回调函数
void framebuffer_size_callback(GLFWwindow *, int, int);

float deltaTime = 0.0f;  // Time between current frame and last frame
float lastTime = 0.0f;  // Time of last frame

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);  // 相机位置
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // 用于定义相机朝向
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // 相机上方向

// 用于存储渲染结果的内存缓冲区，可以将缓冲区的内容保存到硬盘
GLubyte pixels[3 * 1200 * 1600];


// 颜色贴图文件的位置
#define COLOR_TEXTURE_IMAGE_FILE_LOCATION "./img/lroc_color_poles_2k.png"
// 位移贴图文件的位置
#define DISPLACEMENT_TEXTURE_IMAGE_FILE_LOCATION "./img/ldem_3_8bit.jpg"

// 窗口的初始宽度
#define WINDOW_WIDTH 1600
// 窗口的初始高度
#define WINDOW_HEIGHT 1200


// 保存当前窗口的宽度
unsigned int window_width = WINDOW_WIDTH;
// 保存当前窗口的高度
unsigned int window_height = WINDOW_HEIGHT;

#define DEPTH_MAP_WIDTH_FROM_LIGHT_SOURCE 1024
#define DEPTH_MAP_HEIGHT_FROM_LIGHT_SOURCE 1024


// 光源视角下进行渲染使用的顶点着色器的源文件
#define VERTEX_SHADER_LIGHT_SOURCE_FILE_LOCATION "./src/light_vertex.glsl"
// 光源视角下进行渲染使用的片段着色器的源文件
#define FRAGMENT_SHADER_LIGHT_SOURCE_FILE_LOCATION "./src/light_fragment.glsl"
// 摄像机视角下渲染月球使用的顶点着色器的源文件
#define VERTEX_SHADER_CAMERA_SOURCE_FILE_LOCATION "./src/vertex.glsl"
// 摄像机视角下渲染月球使用的片段着色器的源文件
#define FRAGMENT_SHADER_CAMERA_SOURCE_FILE_LOCATION "./src/fragment.glsl"

/*
从光源视角下获取深度信息时，所使用的正交投影所需的投影视锥的参数
投影视锥的边界。月球表面存在起伏，如果直接把投影视锥设置为球体的直径，
会丢失部分的阴影。由于位移贴图的比例是根据月球半径进行调整的，若月球
半径变化时，需要同步调节投影视锥的边界值
*/
#define ORTHOGONAL_FRUSTUM_BORDER 0.2f
#define ORTHOGONAL_FRUSTUM_LEFT -(RADIUS + ORTHOGONAL_FRUSTUM_BORDER)
#define ORTHOGONAL_FRUSTUM_RIGHT RADIUS + ORTHOGONAL_FRUSTUM_BORDER
#define ORTHOGONAL_FRUSTUM_TOP RADIUS + ORTHOGONAL_FRUSTUM_BORDER
#define ORTHOGONAL_FRUSTUM_BOTTOM -(RADIUS + ORTHOGONAL_FRUSTUM_BORDER)
// 光源修改时，需要同步修改远近平面的值
// 近平面
#define NEAR_PLANE 1.0f
// 远平面
#define FAR_PLANE 30.0f

// 光源环绕半径
#define LIGHT_RADIUS 10.0f

// 每次旋转步长，单位：1 / 2\pi
#define ROTATION_STEP 4320

int main(void)
{
    Sphere sphere = Sphere(RADIUS, LONGITUDE_GRANULARITY, LATITUDE_GRANULARITY);  // 实例化简单球面对象
    std::vector<float> & vertices = sphere.get_vertices();  // 获取球面顶点数组
    std::vector<unsigned int> & indices = sphere.get_indices();  // 获取球面三角形顶点索引

    if (!glfwInit())
    {
        std::cout << "Initialize GLFW Failed." << std::endl;
        exit(-1);
    }
    else
    {
        // 设置OpenGL版本号
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        // 设置渲染模式为核心渲染模式
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // 创建一个窗口
        GLFWwindow * window = glfwCreateWindow(window_width, window_height, "Moon", NULL, NULL);
        if (window == NULL)
        {
            std::cout << "Fail to create GLFW window" << std::endl;
            glfwTerminate();
            exit(-1);
        }
        glfwMakeContextCurrent(window);

        // 查询OpenGL函数地址
        if (!gladLoadGL(glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            glfwTerminate();
            exit(-1);
        }

        // 设置窗口大小改变时调用的回调函数
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        // 创建并绑定一个顶点数组对象，用于管理顶点缓冲对象VBO和索引缓冲对象EBO
        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // 创建并绑定一个顶点缓冲对象，用于管理存储在显存中的顶点数据
        unsigned int VBO;
        glGenBuffers(1, &VBO);
        // 顶点缓冲对象的缓冲类型是GL_ARRAY_BUFFER
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // 将顶点数据复制到当前绑定的顶点缓冲中，因为顶点数据不会发生变动，所以使用GL_STATIC_DRAW
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
        
        // 创建并绑定一个索引缓冲对象，用于管理存储在显存中的索引数据
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        // 顶点索引对象的缓冲类型是GL_ELEMENT_ARRAY_BUFFER
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // 将索引数据复制到当前绑定的索引缓冲中，因为索引数据不会发生变动，所以使用GL_STATIC_DRAW
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), &indices[0], GL_STATIC_DRAW);

        // 设置顶点属性，位置[0, 1, 2]分别是顶点的[x, y z]坐标
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        // 设置顶点属性，位置[3， 4]分别是顶点的[u, v]坐标
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // 设置顶点属性，位置[5, 6, 7]分别是顶点沿着u方向的切向量[u, v, w]
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // 设置顶点属性，位置[8, 9, 10]分别是顶点沿着v方向的切向量[u, v, w]
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void *)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        // int number;
        // glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &number);
        // std::cout << "Maximum number of vertex attributes supported: " << number << std::endl;

        // 加载颜色贴图
        int color_texture_image_width, color_texture_image_height, color_texture_image_channels;
        unsigned char * color_texture_image_data = stbi_load(COLOR_TEXTURE_IMAGE_FILE_LOCATION, &color_texture_image_width, &color_texture_image_height, &color_texture_image_channels, 0);

        // 创建并绑定一个纹理对象，用于设置颜色贴图
        unsigned int color_texture;
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &color_texture);
        glBindTexture(GL_TEXTURE_2D, color_texture);
        if (color_texture_image_data)
        {
            // 将颜色贴图附加到绑定的纹理对象上
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, color_texture_image_width, color_texture_image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, color_texture_image_data);
            // 生成Mipmap
            glGenerateMipmap(GL_TEXTURE_2D);
            // 释放纹理图像占用的内存
            stbi_image_free(color_texture_image_data);
        }
        else
        {
            std::cout << "Fail to load color texture" << std::endl;
            // 释放纹理图像占用的内存
            stbi_image_free(color_texture_image_data);
            glfwTerminate();
            exit(-1);
        }

        // Set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

         // 加载位移贴图
        int displacement_texture_image_width, displacement_texture_image_height, displacement_texture_image_channels;
        unsigned char * displacement_texture_image_data = stbi_load(DISPLACEMENT_TEXTURE_IMAGE_FILE_LOCATION, &displacement_texture_image_width, &displacement_texture_image_height, &displacement_texture_image_channels, 0);

        // 创建并绑定一个纹理对象，用于设置位移贴图
        unsigned int displacement_texture;
        glActiveTexture(GL_TEXTURE1);
        glGenTextures(1, &displacement_texture);
        glBindTexture(GL_TEXTURE_2D, displacement_texture);
        if (displacement_texture_image_data)
        {
            // 将位移贴图附加到绑定的纹理对象上
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, displacement_texture_image_width, displacement_texture_image_height, 0, GL_RED, GL_UNSIGNED_BYTE, displacement_texture_image_data);
            // 生成Mipmap
            glGenerateMipmap(GL_TEXTURE_2D);
            // 释放纹理图像占用的内存
            stbi_image_free(displacement_texture_image_data);
        }
        else
        {
            std::cout << "Fail to load displacement texture" << std::endl;
            // 释放纹理图像占用的内存
            stbi_image_free(displacement_texture_image_data);
            glfwTerminate();
            exit(-1);
        }

        // Set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 创建并绑定一个帧缓冲对象
        unsigned int depth_map_fbo_for_light_source;
        glGenFramebuffers(1, &depth_map_fbo_for_light_source);
        glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_for_light_source);

         // 创建并绑定一个纹理对象，用于保存光源视角下深度信息
        unsigned int depth_map_from_light_source;
        glActiveTexture(GL_TEXTURE2);
        glGenTextures(1, &depth_map_from_light_source);
        glBindTexture(GL_TEXTURE_2D, depth_map_from_light_source);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DEPTH_MAP_WIDTH_FROM_LIGHT_SOURCE, DEPTH_MAP_HEIGHT_FROM_LIGHT_SOURCE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        
        // Set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // 将生成的深度纹理对象作为帧缓冲的深度缓冲
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_map_from_light_source, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        // 重接绑定默认的帧缓冲器
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 实例化一个Shader类对象，用于渲染光源视角下的深度图
        Shader depth_from_light_source_shader = Shader(VERTEX_SHADER_LIGHT_SOURCE_FILE_LOCATION, FRAGMENT_SHADER_LIGHT_SOURCE_FILE_LOCATION);
        // 实例化一个Shader类对象，用于渲染最终的月球
        Shader shader = Shader(VERTEX_SHADER_CAMERA_SOURCE_FILE_LOCATION, FRAGMENT_SHADER_CAMERA_SOURCE_FILE_LOCATION);

        // 获取uniform变量的位置
        unsigned int radius_in_light_shader = glGetUniformLocation(depth_from_light_source_shader.id, "radius");
        unsigned int light_model_matrix_location = glGetUniformLocation(depth_from_light_source_shader.id, "model_matrix");
        unsigned int light_displacement_texture_location = glGetUniformLocation(depth_from_light_source_shader.id, "displacement_texture");
        unsigned int light_space_matrix_in_light_shader = glGetUniformLocation(depth_from_light_source_shader.id, "light_space_matrix");

        depth_from_light_source_shader.use();
        // 设置位移贴图位置
        glUniform1i(light_displacement_texture_location, 1);
        // 设置球体半径参数
        glUniform1f(radius_in_light_shader, RADIUS);
        // 光源视角下的正交投影视锥矩阵
        glm::mat4 light_projection_matrix = glm::ortho(ORTHOGONAL_FRUSTUM_LEFT, ORTHOGONAL_FRUSTUM_RIGHT, ORTHOGONAL_FRUSTUM_BOTTOM, ORTHOGONAL_FRUSTUM_TOP, NEAR_PLANE, FAR_PLANE);


        // 获取uniform变量的位置
        unsigned int radius_in_shader = glGetUniformLocation(shader.id, "radius");
        unsigned int transform_matrix_location = glGetUniformLocation(shader.id, "transform_matrix");
        unsigned int model_matrix_location = glGetUniformLocation(shader.id, "model_matrix");
        unsigned int view_position_location = glGetUniformLocation(shader.id, "view_position");
        unsigned int light_position_location = glGetUniformLocation(shader.id, "light_position");
        unsigned int color_texture_location = glGetUniformLocation(shader.id, "color_texture");
        unsigned int displacement_texture_location = glGetUniformLocation(shader.id, "displacement_texture");
        unsigned int shadow_map_location = glGetUniformLocation(shader.id, "shadow_map");
        unsigned int lightSpaceMatrixLoc_in_shader = glGetUniformLocation(shader.id, "light_space_matrix");
        
        shader.use();
        // 分布设置颜色、位移、阴影贴图位置
        glUniform1i(color_texture_location, 0);
        glUniform1i(displacement_texture_location, 1);
        glUniform1i(shadow_map_location, 2);
        //设置球体半径参数
        glUniform1f(radius_in_shader, RADIUS);

        // 弧度
        float rad = 0.0;
        // 计数器
        int counter = 0;

        glEnable(GL_DEPTH_TEST);
        while (!glfwWindowShouldClose(window))
        {
            // 处理键盘事件
            process_input(window);

            // 更新旋转弧度
            rad = M_PI * 2 / ROTATION_STEP * counter;

            // 生成模型变换矩阵
            glm::mat4 model_matrix = glm::mat4(1.0f);
            model_matrix = glm::rotate(model_matrix, rad, glm::vec3(0.0f, 1.0f, 0.0f));

            // 生成光源坐标
            float light_x = sin(rad / 2) * LIGHT_RADIUS;
            float light_z = cos(rad / 2) * LIGHT_RADIUS;
            glm::vec3 light_position = glm::vec3(light_x, 0.0f, light_z);

            // 生成光源视角矩阵和总变换矩阵
            glm::mat4 light_view_matrix = glm::lookAt(light_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 light_space_matrix = light_projection_matrix * light_view_matrix;

            depth_from_light_source_shader.use();
            glViewport(0, 0, DEPTH_MAP_WIDTH_FROM_LIGHT_SOURCE, DEPTH_MAP_HEIGHT_FROM_LIGHT_SOURCE);
            glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_for_light_source);
            glClear(GL_DEPTH_BUFFER_BIT);

            glUniformMatrix4fv(light_space_matrix_in_light_shader, 1, GL_FALSE, glm::value_ptr(light_space_matrix));
            glUniformMatrix4fv(light_model_matrix_location, 1, GL_FALSE, glm::value_ptr(model_matrix));

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);


            // 生成相机视角矩阵
            glm::mat4 view_matrix;
            view_matrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            // 生成相机透视投影矩阵
            glm::mat4 projection_matrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
            // 生成总变换矩阵
            glm::mat4 total = projection_matrix * view_matrix * model_matrix;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            shader.use();
            glViewport(0, 0, window_width, window_height);

            glfwSwapBuffers(window);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUniform3fv(light_position_location, 1, glm::value_ptr(light_position));
            glUniformMatrix4fv(model_matrix_location, 1, GL_FALSE, glm::value_ptr(model_matrix));
            glUniformMatrix4fv(transform_matrix_location, 1, GL_FALSE, glm::value_ptr(total));
            glUniformMatrix4fv(lightSpaceMatrixLoc_in_shader, 1, GL_FALSE, glm::value_ptr(light_space_matrix));
            glUniform3fv(view_position_location, 1, glm::value_ptr(cameraPos));
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            
            // glReadBuffer(GL_FRONT);
            // std::string s = "./export/img_";
            // s += std::to_string(counter);
            // s += ".png";
            // glReadPixels(0, 0, 1600, 1200, GL_RGB, GL_UNSIGNED_BYTE, pixels);
            // stbi_write_png(s.c_str(), 1600, 1200, 3, pixels, 0);
            
            glfwPollEvents();
            float currentTime = glfwGetTime();
            deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            counter++;
        }

    
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    return 0;
}

void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
}

void process_input(GLFWwindow * window)
{
    float cameraSpeed = deltaTime * 2.5f;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}
