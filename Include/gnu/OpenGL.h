#pragma once

#define GLM_ENABLE_EXPERIMENTAL 
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <map> 
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

using GLuint = unsigned int;
using GLfloat = float; 
struct GLFWwindow;

namespace tinyobj {
    struct index_t;
}

namespace gnu {
    struct PEGLVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
    };

    struct PEGLShaderProgram {
        GLuint programID = 0;
    };

    struct PEGLMesh {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        uint32_t indexCount = 0;

        glm::vec3 baseColor{ 1.0f, 1.0f, 1.0f };
        GLuint textureID = 0;

        glm::vec3 position{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

        glm::mat4 get_transform() const;
        void translate(const glm::vec3& offset);
        void rotate(const glm::quat& delta_rotation);
        void set_scale(const glm::vec3& new_scale);
        void delete_gpu_resources();
    };

    struct PEGLModel {
        glm::vec3 model_position{ 0.0f };
        std::vector<PEGLMesh> meshes;
    };

    struct PEGLLightSource {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 color{ 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;

        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

        glm::mat4 get_transform() const;
    };

    struct PEGLtinyobj_index_cmp {
        bool operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const;
    };

    GLFWwindow* PEGLInit_OpenGL_Window(int width, int height, const std::string& title);
    void PEGLShow_Loading_Screen(GLFWwindow* window,const gnu::PEGLShaderProgram& uiShader,const glm::mat4& orthoMatrix);
    PEGLShaderProgram* PEGLAudo_Compile_and_Link_Shader();
    PEGLShaderProgram PEGLCompile_and_Link_Shader(const std::string& vertexPath,const std::string& fragmentPath);
    GLuint PEGLLoad_Texture_From_File(const std::string& filePath);
    void PEGLPrepare_Mesh_For_GPU(PEGLMesh& mesh, const std::vector<PEGLVertex>& vertices,const std::vector<uint32_t>& indices);
    PEGLModel PEGLLoad_Model_From_File_OBJ(const std::string& filePath, const std::string& baseDir);
    PEGLModel PEGLCreate_Cube_Model();
    void PEGLDraw_Mesh(const PEGLMesh& mesh, const PEGLShaderProgram& shader, const glm::mat4& viewProjection);
    void PEGLDraw_Model(const PEGLModel& model, const PEGLShaderProgram& shader, const glm::mat4& viewProjection);
    PEGLLightSource PEGLCreate_Light_Source(const glm::vec3& position, const glm::vec3& color, float intensity);
    void PEGLDelete_Model(PEGLModel& model);
    void PEGLDelete_Shader_Program(PEGLShaderProgram& program);

    namespace UI {
        struct PEGLUIQuad {
            glm::vec2 position{ 0.0f };
            glm::vec2 size{ 100.0f, 30.0f };
            glm::vec3 color{ 0.7f, 0.7f, 0.7f };
            PEGLMesh mesh;
            float rotation = 0.0f;
            float layer = 0.0f;

            glm::mat4 get_transform() const;
        };

        struct PEGLButton : public PEGLUIQuad {
            std::string text = "Button";
            glm::vec3 textColor{ 0.0f, 0.0f, 0.0f };
            bool isHovered = false;
        };

        struct PEGLCheckbox : public PEGLUIQuad {
            glm::vec2 boxSize{ 20.0f, 20.0f };
            bool isChecked = false;
            std::string text = "Checkbox";
        };

        struct PEGLInputField : public PEGLUIQuad {
            std::string text = "";
            std::string hintText = "Enter text...";
            glm::vec3 textColor{ 0.0f, 0.0f, 0.0f };
            bool isActive = false;
        };

        struct PEGLImage : public PEGLUIQuad {
            GLuint textureID = 0; // ID текстуры для отрисовки.
        };

        struct PEGLPanel : public PEGLUIQuad {
        };

        PEGLMesh PEGLCreate_Quad_Mesh();

        void PEGLDraw_Quad_2D(const PEGLUIQuad& quad, const PEGLShaderProgram& shader, const glm::mat4& orthoMatrix);

        void PEGLDraw_Button(const PEGLButton& button, const PEGLShaderProgram& uiShader, const PEGLShaderProgram& textShader, const glm::mat4& orthoMatrix);
        void PEGLDraw_Checkbox(const PEGLCheckbox& checkbox, const PEGLShaderProgram& uiShader, const PEGLShaderProgram& textShader, const glm::mat4& orthoMatrix);
        void PEGLDraw_InputField(const PEGLInputField& input, const PEGLShaderProgram& uiShader, const PEGLShaderProgram& textShader, const glm::mat4& orthoMatrix);
        void PEGLDraw_Image(const PEGLImage& image, const PEGLShaderProgram& uiShader, const glm::mat4& orthoMatrix);
        void PEGLDraw_Panel(const PEGLPanel& panel, const PEGLShaderProgram& uiShader, const glm::mat4& orthoMatrix);
        bool PEGLis_point_in_quad(float x, float y, const PEGLUIQuad& quad);
        float PEGLprint_string_get_width(const char* text, float scale_x = 1.0f);
        void PEGLprint_string(float x, float y, const char* text, float r, float g, float b,
            float scale_x = 1.0f, bool flip_y = true);
    }
}