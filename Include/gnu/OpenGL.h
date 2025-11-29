#pragma once

// --- РАЗРЕШАЕМ ЭКСПЕРИМЕНТАЛЬНЫЕ РАСШИРЕНИЯ GLM (Устраняет #error) ---
#define GLM_ENABLE_EXPERIMENTAL 
// ----------------------------------------------------------------------

#include <glad/glad.h>   // Основные функции OpenGL
#include <GLFW/glfw3.h>  // Управление окном и контекстом

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp> 

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <map> 
#include <tiny_obj_loader.h>
#include <stb_easy_font.h> 

namespace gnu {
    struct Vertex {
        glm::vec3 position; // Позиция (x, y, z)
        glm::vec3 normal;   // Нормаль (для освещения)
        glm::vec2 texCoords; // Координаты текстуры (u, v)
    };

    struct ShaderProgram {
        GLuint programID = 0;
    };

    struct Mesh {
        GLuint VAO = 0; // Vertex Array Object
        GLuint VBO = 0; // Vertex Buffer Object
        GLuint EBO = 0; // Element Buffer Object (Индексы)
        uint32_t indexCount = 0;

        glm::vec3 baseColor{ 1.0f, 1.0f, 1.0f };
        GLuint textureID = 0;

        // Трансформации
        glm::vec3 position{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

        glm::mat4 get_transform() const;
        void translate(const glm::vec3& offset);
        void rotate(const glm::quat& delta_rotation);
        void set_scale(const glm::vec3& new_scale);
        void delete_gpu_resources();
    };

    struct Model {
        glm::vec3 model_position{ 0.0f };
        std::vector<Mesh> meshes;
    };

    struct LightSource {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 color{ 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;

        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

        glm::mat4 get_transform() const;
    };

    // Кастомный компаратор для tinyobj::index_t
    struct tinyobj_index_cmp {
        bool operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const {
            if (a.vertex_index != b.vertex_index) return a.vertex_index < b.vertex_index;
            if (a.normal_index != b.normal_index) return a.normal_index < b.normal_index;
            return a.texcoord_index < b.texcoord_index;
        }
    };

    // --- ОСНОВНЫЕ ФУНКЦИИ ДВИЖКА ---
    GLFWwindow* Init_OpenGL_Window(int width, int height, const std::string& title);
    ShaderProgram Compile_and_Link_Shader(const std::string& vertexPath,
        const std::string& fragmentPath);
    GLuint Load_Texture_From_File(const std::string& filePath);
    void Prepare_Mesh_For_GPU(Mesh& mesh,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices);
    Model Load_Model_From_File_OBJ(const std::string& filePath, const std::string& baseDir);
    Model Create_Cube_Model();
    void Draw_Mesh(const Mesh& mesh, const ShaderProgram& shader, const glm::mat4& viewProjection);
    void Draw_Model(const Model& model, const ShaderProgram& shader, const glm::mat4& viewProjection);
    LightSource Create_Light_Source(const glm::vec3& position, const glm::vec3& color, float intensity);

    // ⭐️ НОВОЕ: Пространство имен для функций 2D UI ⭐️
    namespace UI {
        // ⭐️ Базовая структура для всех 2D-элементов (с поддержкой слоев и вращения) ⭐️
        struct UIQuad {
            glm::vec2 position{ 0.0f };      // Позиция верхнего левого угла
            glm::vec2 size{ 100.0f, 30.0f }; // Ширина и высота
            glm::vec3 color{ 0.7f, 0.7f, 0.7f }; // Цвет фона/элемента
            Mesh mesh;                       // Геометрия (один квадрат)

            // ⭐️ FIX: Добавлен rotation ⭐️
            float rotation = 0.0f;           // Вращение (в градусах)

            // Система слоев (0 - нижний, 1000 - верхний)
            float layer = 0.0f;

            // Вычисление матрицы трансформации (использует layer для Z-координаты)
            glm::mat4 get_transform() const;
        };

        // 1. BUTTON
        struct Button : public UIQuad {
            std::string text = "Button";
            glm::vec3 textColor{ 0.0f, 0.0f, 0.0f };
            bool isHovered = false;
        };

        // 2. CHECKBOX
        struct Checkbox : public UIQuad {
            glm::vec2 boxSize{ 20.0f, 20.0f };
            bool isChecked = false;
            std::string text = "Checkbox";
        };

        // 3. INPUT FIELD
        struct InputField : public UIQuad {
            std::string currentText = "";
            std::string hintText = "Enter text...";
            glm::vec3 textColor{ 0.0f, 0.0f, 0.0f };
            bool isActive = false;
        };

        // 4. IMAGE (ИЗОБРАЖЕНИЕ/ТЕКСТУРА)
        struct Image : public UIQuad {
            GLuint textureID = 0; // ID текстуры для отрисовки.
        };

        // 5. PANEL (ПАНЕЛЬ/ФОН)
        struct Panel : public UIQuad {
            // Наследует все: position, size, color, layer, rotation.
        };


        // --- ФУНКЦИИ УПРАВЛЕНИЯ UI ---

        Mesh Create_Quad_Mesh();

        // Общая функция отрисовки квада (используется всеми элементами)
        void Draw_Quad_2D(const UIQuad& quad, const ShaderProgram& shader, const glm::mat4& orthoMatrix);

        // Отрисовка специфических элементов
        void Draw_Button(const Button& button, const ShaderProgram& uiShader, const ShaderProgram& textShader, const glm::mat4& orthoMatrix);
        void Draw_Checkbox(const Checkbox& checkbox, const ShaderProgram& uiShader, const ShaderProgram& textShader, const glm::mat4& orthoMatrix);
        void Draw_InputField(const InputField& input, const ShaderProgram& uiShader, const ShaderProgram& textShader, const glm::mat4& orthoMatrix);

        // Новые функции отрисовки
        void Draw_Image(const Image& image, const ShaderProgram& uiShader, const glm::mat4& orthoMatrix);
        void Draw_Panel(const Panel& panel, const ShaderProgram& uiShader, const glm::mat4& orthoMatrix);


        // Вспомогательные функции для текста
        bool is_point_in_quad(float x, float y, const UIQuad& quad);
        float print_string_get_width(const char* text, float scale_x = 1.0f);
        void print_string(float x, float y, const char* text, float r, float g, float b,
            float scale_x = 1.0f, bool flip_y = true);
    }
}