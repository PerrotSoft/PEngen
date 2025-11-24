#pragma once

// --- РАЗРЕШАЕМ ЭКСПЕРИМЕНТАЛЬНЫЕ РАСШИРЕНИЯ GLM (Устраняет #error) ---
#define GLM_ENABLE_EXPERIMENTAL 
// ----------------------------------------------------------------------

#include <glad/glad.h>   // Основные функции OpenGL (замена GLEW)
#include <GLFW/glfw3.h>  // Управление окном и контекстом

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // Здесь используется экспериментальное расширение
#include <glm/gtx/rotate_vector.hpp> // (Если вы его используете)

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <map> // ⭐️ ДОБАВЛЕНО: Необходимо для std::map
// ⭐️ НОВЫЙ #include ДЛЯ ЗАГРУЗКИ МОДЕЛЕЙ ⭐️
#include <tiny_obj_loader.h>

namespace gnu {
    struct Vertex {
        glm::vec3 position; // Позиция (x, y, z)
        glm::vec3 normal;   // Нормаль (для освещения)
        glm::vec2 texCoords; // Координаты текстуры (u, v)
    };
    struct tinyobj_index_cmp {
        bool operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const {
            if (a.vertex_index != b.vertex_index) return a.vertex_index < b.vertex_index;
            if (a.normal_index != b.normal_index) return a.normal_index < b.normal_index;
            return a.texcoord_index < b.texcoord_index;
        }
    };
    struct Mesh {
        // --- 1. ТРАНСФОРМАЦИЯ ---
        glm::vec3 position{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

        GLuint VAO = 0; // Vertex Array Object (Сборка буферов)
        GLuint VBO = 0; // Vertex Buffer Object
        GLuint EBO = 0; // Element Buffer Object (Индексы)
        uint32_t indexCount = 0; // Сколько индексов нужно нарисовать

        glm::vec3 baseColor{ 1.0f, 1.0f, 1.0f };
        GLuint textureID = 0; // ID загруженной текстуры (0, если нет)
        glm::mat4 get_transform() const;
        void translate(const glm::vec3& offset);
        void rotate(const glm::quat& delta_rotation);
        void set_scale(const glm::vec3& new_scale);

        // Очистка ресурсов OpenGL
        void delete_gpu_resources();
    };
    struct Model {
        glm::vec3 model_position{ 0.0f };
        std::vector<Mesh> meshes;
    };
    struct ShaderProgram {
        GLuint programID = 0;
    };
    GLFWwindow* Init_OpenGL_Window(int width, int height, const std::string& title);
    void Prepare_Mesh_For_GPU(Mesh& mesh,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices);
    GLuint Load_Texture_From_File(const std::string& filePath);
    ShaderProgram Compile_and_Link_Shader(const std::string& vertexPath,
        const std::string& fragmentPath);
    Model Load_Model_From_File(const std::string& filePath);
    void Draw_Mesh(const Mesh& mesh, const ShaderProgram& shader, const glm::mat4& viewProjection);
    void Draw_Model(const Model& model, const ShaderProgram& shader, const glm::mat4& viewProjection);

} // Закрытие namespace gnu