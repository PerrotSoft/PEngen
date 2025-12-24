#include "../../Include/gnu/OpenGL.h"

#define P_ENGEN_GLM_STUB_ACTIVE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <tiny_obj_loader.h>
#include <stb_easy_font.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include <map> 
#include <cmath> 
#include "../../Include/cfg/log.h"
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> 
#include <stb_easy_font.h> 
#ifdef _WIN32
#define SHADERS_BASE_PATH "PEngen\\shaders\\"
#define BASE_PATH_ICON_PE "PEngen\\data\\icon.png"
#else
#define SHADERS_BASE_PATH "PEngen/shaders/"
#define BASE_PATH_ICON_PE "PEngen/data/icon.png"
#endif
namespace gnu {
    struct ImageData {
        int width = 0;
        int height = 0;
        int components = 0;
        std::vector<unsigned char> data;
    };

    glm::mat4 PEGLMesh::get_transform() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = model * glm::toMat4(rotation);
        model = glm::scale(model, scale);
        return model;
    }
    glm::mat4 PEGLLightSource::get_transform() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = model * glm::toMat4(rotation);
        model = glm::scale(model, scale);
        return model;
    }
    void PEGLMesh::translate(const glm::vec3& offset) {
        position += offset;
    }

    void PEGLMesh::rotate(const glm::quat& delta_rotation) {
        rotation = delta_rotation * rotation;
    }

    void PEGLMesh::set_scale(const glm::vec3& new_scale) {
        scale = new_scale;
    }

    void PEGLMesh::delete_gpu_resources() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
        VAO = VBO = EBO = 0;
        indexCount = 0;
    }

    void PEGLDelete_Shader_Program(PEGLShaderProgram& program) {
        if (program.programID != 0) {
            glDeleteProgram(program.programID);
            program.programID = 0;
        }
    }

    void PEGLDelete_Model(PEGLModel& model) {
        for (auto& mesh : model.meshes) {
            if (mesh.VAO != 0) glDeleteVertexArrays(1, &mesh.VAO);
            if (mesh.VBO != 0) glDeleteBuffers(1, &mesh.VBO);
            if (mesh.EBO != 0) glDeleteBuffers(1, &mesh.EBO);

            mesh.VAO = mesh.VBO = mesh.EBO = mesh.textureID = 0;
            mesh.indexCount = 0;
        }
        model.meshes.clear();
    }

    static std::string Read_File(const std::string& filePath) {
        std::ifstream fileStream(filePath, std::ios::in);
        if (!fileStream.is_open()) {
            throw std::runtime_error("Could not open file: " + filePath);
        }
        std::stringstream sstr;
        sstr << fileStream.rdbuf();
        fileStream.close();
        return sstr.str();
    }

    static GLuint Compile_Shader(GLuint type, const std::string& source) {
        GLuint shaderID = glCreateShader(type);
        const char* sourcePtr = source.c_str();
        glShaderSource(shaderID, 1, &sourcePtr, NULL);
        glCompileShader(shaderID);

        GLint result = GL_FALSE;
        int infoLogLength;
        glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

        if (infoLogLength > 0 && result == GL_FALSE) {
            std::vector<char> errorMessage(infoLogLength + 1);
            glGetShaderInfoLog(shaderID, infoLogLength, NULL, &errorMessage[0]);
            glDeleteShader(shaderID);
            throw std::runtime_error(std::string(errorMessage.begin(), errorMessage.end()));
        }
        return shaderID;
    }

    PEGLShaderProgram PEGLCompile_and_Link_Shader(const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        PEGLShaderProgram program;
        std::string fullVertPath = SHADERS_BASE_PATH + vertexPath;
        std::string fullFragPath = SHADERS_BASE_PATH + fragmentPath;

        try {
            std::string vertexCode = Read_File(fullVertPath);
            std::string fragmentCode = Read_File(fullFragPath);

            GLuint vertexID = Compile_Shader(GL_VERTEX_SHADER, vertexCode);
            GLuint fragmentID = Compile_Shader(GL_FRAGMENT_SHADER, fragmentCode);

            program.programID = glCreateProgram();
            glAttachShader(program.programID, vertexID);
            glAttachShader(program.programID, fragmentID);
            glLinkProgram(program.programID);

            GLint result = GL_FALSE;
            int infoLogLength;
            glGetProgramiv(program.programID, GL_LINK_STATUS, &result);
            glGetProgramiv(program.programID, GL_INFO_LOG_LENGTH, &infoLogLength);

            if (infoLogLength > 0 && result == GL_FALSE) {
                std::vector<char> errorMessage(infoLogLength + 1);
                glGetProgramInfoLog(program.programID, infoLogLength, NULL, &errorMessage[0]);
                glDeleteProgram(program.programID);
                program.programID = 0;
                PElogger((std::string("Shader Linking Failed: ") + std::string(errorMessage.begin(), errorMessage.end())).c_str());
                throw std::runtime_error("Shader Linking Failed: " + std::string(errorMessage.begin(), errorMessage.end()));
            }

            glDetachShader(program.programID, vertexID);
            glDetachShader(program.programID, fragmentID);
            glDeleteShader(vertexID);
            glDeleteShader(fragmentID);
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Shader Error: " << e.what() << std::endl;
            PElogger((std::string("Shader Error: ") + e.what()).c_str());
            program.programID = 0;
        }
        return program;
    }
    static void glfw_error_callback(int error, const char* description) {
        std::cerr << "GLFW Error " << error << ": " << description << std::endl;
    }
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }
    PEGLShaderProgram* PEGLAudo_Compile_and_Link_Shader() {
        PEGLShaderProgram modelShader = PEGLCompile_and_Link_Shader("simple.vert", "simple.frag");

        PEGLShaderProgram uiShader = PEGLCompile_and_Link_Shader("simple.vert", "simple.frag");

        PEGLShaderProgram textShader = PEGLCompile_and_Link_Shader("text.vert", "text.frag");
	    return new PEGLShaderProgram[3]{ modelShader, uiShader, textShader };
    }

    void PEGLShow_Loading_Screen(GLFWwindow* window,
        const PEGLShaderProgram& uiShader,
        const glm::mat4& orthoMatrix)
    {
        if (!window || uiShader.programID == 0) {
            PElogger("ERROR: Failed to initialize loading screen (Window or Shader is NULL).");
            return;
        }
        GLuint splashTextureID = PEGLLoad_Texture_From_File(BASE_PATH_ICON_PE);

        if (splashTextureID == 0) {
            PElogger((std::string("WARNING: Failed to load splash screen texture: ") + BASE_PATH_ICON_PE).c_str());
            return;
        }
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        UI::PEGLImage splashImage;
        static PEGLMesh staticQuadPEGLMesh = UI::PEGLCreate_Quad_Mesh();

        splashImage.mesh = staticQuadPEGLMesh;
        splashImage.textureID = splashTextureID;
        splashImage.position = glm::vec2(0.0f, 0.0f);
        splashImage.size = glm::vec2((float)windowWidth, (float)windowHeight);
        splashImage.layer = 1.0f;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UI::PEGLDraw_Image(splashImage, uiShader, orthoMatrix);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    GLFWwindow* PEGLInit_OpenGL_Window(int width, int height, const std::string& title) {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            PElogger("Failed to initialize GLFW");
            return nullptr;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
        if (!window) {
            std::cerr << "Failed to open GLFW window." << std::endl;
            PElogger("Failed to open GLFW window.");
            glfwTerminate();
            return nullptr;
        }
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            PElogger("Failed to initialize GLAD");
            glfwDestroyWindow(window);
            glfwTerminate();
            return nullptr;
        }
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        return window;
    }
    GLuint PEGLLoad_Texture_From_File(const std::string& filePath) {
        GLuint textureID = 0;
        int width, height, comp;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &comp, 0);

        if (data) {
            GLenum format = GL_RGB;
            if (comp == 4) format = GL_RGBA;
            if (comp == 1) format = GL_RED;

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(data);
        }
        else {
            std::cerr << "ERROR: STB_IMAGE failed to load texture: " << filePath << std::endl;
		    PElogger(("ERROR: STB_IMAGE failed to load texture: " + filePath).c_str());
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        return textureID;
    }
    void PEGLPrepare_Mesh_For_GPU(PEGLMesh& PEGLMesh,
        const std::vector<PEGLVertex>& vertices,
        const std::vector<uint32_t>& indices)
    {
        PEGLMesh.delete_gpu_resources();

        glGenVertexArrays(1, &PEGLMesh.VAO);
        glGenBuffers(1, &PEGLMesh.VBO);
        glGenBuffers(1, &PEGLMesh.EBO);

        glBindVertexArray(PEGLMesh.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, PEGLMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(PEGLVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PEGLMesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PEGLVertex), (void*)offsetof(PEGLVertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PEGLVertex), (void*)offsetof(PEGLVertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PEGLVertex), (void*)offsetof(PEGLVertex, texCoords));

        glBindVertexArray(0);

        PEGLMesh.indexCount = (uint32_t)indices.size();
    }

    PEGLModel PEGLLoad_Model_From_File_OBJ(const std::string& filePath, const std::string& baseDir) {
        PEGLModel model;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err, warn;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), baseDir.c_str());

        if (!warn.empty()) {
            std::cout << "tinyobjloader WARNING: " << warn << std::endl;
		    PElogger(("tinyobjloader WARNING: " + warn).c_str());
        }
        if (!err.empty()) {
            throw std::runtime_error("tinyobjloader ERROR: " + err);
		    PElogger(("tinyobjloader ERROR: " + err).c_str());
        }
        if (!ret) {
            throw std::runtime_error("Failed to load/parse OBJ file: " + filePath);
		    PElogger(("Failed to load/parse OBJ file: " + filePath).c_str());
        }

        std::map<tinyobj::index_t, uint32_t, PEGLtinyobj_index_cmp> unique_indices;
        std::vector<PEGLVertex> vertices;
        std::vector<uint32_t> indices;
        for (const auto& shape : shapes) {
            unique_indices.clear();
            vertices.clear();
            indices.clear();

            for (const auto& index : shape.mesh.indices) {
                if (unique_indices.count(index) == 0) {
                    PEGLVertex vertex{};
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };
                    if (index.normal_index >= 0) {
                        vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                        };
                    }
                    if (index.texcoord_index >= 0) {
                        vertex.texCoords = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                        };
                    }

                    uint32_t current_index = (uint32_t)vertices.size();
                    unique_indices[index] = current_index;
                    indices.push_back(current_index);
                    vertices.push_back(vertex);

                }
                else indices.push_back(unique_indices[index]);
            }
            PEGLMesh PEGLMesh;
            PEGLPrepare_Mesh_For_GPU(PEGLMesh, vertices, indices);
            model.meshes.push_back(std::move(PEGLMesh));
        }

        return model;
    }

    PEGLModel PEGLCreate_Cube_Model() {
        PEGLModel model;
        std::vector<PEGLVertex> vertices = {
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}
        };
        std::vector<uint32_t> indices = {
            0, 1, 2, 0, 2, 3,       
            4, 5, 6, 4, 6, 7,       
            8, 9, 10, 8, 10, 11,    
            12, 13, 14, 12, 14, 15, 
            16, 17, 18, 16, 18, 19, 
            20, 21, 22, 20, 22, 23  
        };

        PEGLMesh cubePEGLMesh;
        PEGLPrepare_Mesh_For_GPU(cubePEGLMesh, vertices, indices);
        model.meshes.push_back(std::move(cubePEGLMesh));
        return model;
    }

    PEGLLightSource PEGLCreate_Light_Source(const glm::vec3& position, const glm::vec3& color, float intensity) {
        PEGLLightSource ls;
        ls.position = position;
        ls.color = color;
        ls.intensity = intensity;
        return ls;
    }
    bool PEGLtinyobj_index_cmp::operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const {
        if (a.vertex_index != b.vertex_index) return a.vertex_index < b.vertex_index;
        if (a.normal_index != b.normal_index) return a.normal_index < b.normal_index;
        return a.texcoord_index < b.texcoord_index;
    }
    void PEGLDraw_Mesh(const PEGLMesh& PEGLMesh, const PEGLShaderProgram& shader, const glm::mat4& viewProjection) {
        if (PEGLMesh.VAO == 0) return;

        glUseProgram(shader.programID);

        glm::mat4 model = PEGLMesh.get_transform();
        glm::mat4 MVP = viewProjection * model;

        GLint MVPLoc = glGetUniformLocation(shader.programID, "MVP");
        GLint modelLoc = glGetUniformLocation(shader.programID, "model");
        GLint viewProjectionLoc = glGetUniformLocation(shader.programID, "viewProjection");

        if (MVPLoc != -1) glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, &MVP[0][0]);
        if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        if (viewProjectionLoc != -1) glUniformMatrix4fv(viewProjectionLoc, 1, GL_FALSE, &viewProjection[0][0]);

        GLint useTexLoc = glGetUniformLocation(shader.programID, "useTexture");

        if (PEGLMesh.textureID != 0) {
            if (useTexLoc != -1) glUniform1i(useTexLoc, 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, PEGLMesh.textureID);

            GLint samplerLoc = glGetUniformLocation(shader.programID, "ourTextureSampler");
            if (samplerLoc != -1) glUniform1i(samplerLoc, 0);

        }
        else {
            if (useTexLoc != -1) glUniform1i(useTexLoc, 0);

            GLint colorLoc = glGetUniformLocation(shader.programID, "baseColor");
            if (colorLoc != -1) {
                glUniform3f(colorLoc, PEGLMesh.baseColor.x, PEGLMesh.baseColor.y, PEGLMesh.baseColor.z);
            }
        }
        glBindVertexArray(PEGLMesh.VAO);
        glDrawElements(GL_TRIANGLES, PEGLMesh.indexCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void PEGLDraw_Model(const PEGLModel& model, const PEGLShaderProgram& shader, const glm::mat4& viewProjection) {
        for (const auto& PEGLMesh : model.meshes) {
            PEGLDraw_Mesh(PEGLMesh, shader, viewProjection);
        }
    }
    namespace UI {
        static GLuint textVAO = 0;
        static GLuint textVBO = 0;
        static GLuint textEBO = 0;
        static char textBuffer[90000];
        static std::vector<uint32_t> textIndices;
        static constexpr uint32_t MAX_CHARS = sizeof(textBuffer) / 16 / 4;
        glm::mat4 PEGLUIQuad::get_transform() const {
            float z_position = -this->layer * 0.001f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(position.x, position.y, z_position));
            model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

            return model;
        }
        PEGLMesh PEGLCreate_Quad_Mesh() {
            std::vector<PEGLVertex> vertices = {
                {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
            };
            std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

            PEGLMesh quadPEGLMesh;
            glGenVertexArrays(1, &quadPEGLMesh.VAO);
            glGenBuffers(1, &quadPEGLMesh.VBO);
            glGenBuffers(1, &quadPEGLMesh.EBO);
            glBindVertexArray(quadPEGLMesh.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadPEGLMesh.VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(PEGLVertex), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadPEGLMesh.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PEGLVertex), (void*)offsetof(PEGLVertex, position));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PEGLVertex), (void*)offsetof(PEGLVertex, normal));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(PEGLVertex), (void*)offsetof(PEGLVertex, texCoords));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            quadPEGLMesh.indexCount = indices.size();
            return quadPEGLMesh;
        }
        void PEGLDraw_Quad_2D(const PEGLUIQuad& quad, const PEGLShaderProgram& shader, const glm::mat4& orthoMatrix) {
            if (quad.mesh.VAO == 0 || shader.programID == 0) return;

            glUseProgram(shader.programID);
            glm::mat4 model = quad.get_transform();
            glm::mat4 MVP = orthoMatrix * model;

            GLint MVPLoc = glGetUniformLocation(shader.programID, "MVP");
            if (MVPLoc != -1) glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, &MVP[0][0]);

            GLint useTexLoc = glGetUniformLocation(shader.programID, "useTexture");
            GLint colorLoc = glGetUniformLocation(shader.programID, "baseColor");

            if (quad.mesh.textureID != 0) {
                if (useTexLoc != -1) glUniform1i(useTexLoc, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, quad.mesh.textureID);
                GLint samplerLoc = glGetUniformLocation(shader.programID, "ourTextureSampler");
                if (samplerLoc != -1) glUniform1i(samplerLoc, 0);
            }
            else if (useTexLoc != -1) glUniform1i(useTexLoc, 0);

            if (colorLoc != -1) glUniform3f(colorLoc, quad.color.x, quad.color.y, quad.color.z);
            glBindVertexArray(quad.mesh.VAO);
            glDrawElements(GL_TRIANGLES, quad.mesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        }
        void PEGLDraw_Panel(const PEGLPanel& panel, const PEGLShaderProgram& uiShader, const glm::mat4& orthoMatrix) {
            PEGLUIQuad tempQuad = panel;
            tempQuad.mesh.textureID = 0;
            PEGLDraw_Quad_2D(tempQuad, uiShader, orthoMatrix);
        }
        void PEGLDraw_Image(const PEGLImage& image, const PEGLShaderProgram& uiShader, const glm::mat4& orthoMatrix) {
            if (image.textureID == 0) return;

            PEGLUIQuad tempQuad = image;
            tempQuad.mesh.textureID = image.textureID;
            tempQuad.color = glm::vec3(1.0f, 1.0f, 1.0f);

            PEGLDraw_Quad_2D(tempQuad, uiShader, orthoMatrix);
        }
        float PEGLprint_string_get_width(const char* text, float scale_x) {
            if (text == nullptr) return 0;
            return stb_easy_font_width((char*)text) * scale_x;
        }
        void PEGLprint_string(float x, float y, const char* text, float r, float g, float b,
            float scale_x, bool flip_y)
        {
            static char quad_buffer[99999];
            static char tri_buffer[150000];

            static GLuint textVAO = 0;
            static GLuint textVBO = 0;
            const float CHAR_HEIGHT = 16.0f;
            if (textVAO == 0) {
                glGenVertexArrays(1, &textVAO);
                glGenBuffers(1, &textVBO);
                glBindVertexArray(textVAO);
                glBindBuffer(GL_ARRAY_BUFFER, textVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(tri_buffer), NULL, GL_DYNAMIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16, (void*)0);
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            int num_quads = stb_easy_font_print(x, y, (char*)text, NULL, quad_buffer, sizeof(quad_buffer));

            if (num_quads == 0) return;

            float* src = (float*)quad_buffer;
            float* dst = (float*)tri_buffer;
            int total_vertices = 0;

            for (int i = 0; i < num_quads; ++i) {
                float V1[4], V2[4], V3[4], V4[4];

                V1[0] = *src++; V1[1] = *src++; V1[2] = *src++; V1[3] = *src++;
                V2[0] = *src++; V2[1] = *src++; V2[2] = *src++; V2[3] = *src++;
                V3[0] = *src++; V3[1] = *src++; V3[2] = *src++; V3[3] = *src++;
                V4[0] = *src++; V4[1] = *src++; V4[2] = *src++; V4[3] = *src++;

                float* vertices[] = { V1, V2, V3, V4 };
                for (int j = 0; j < 4; ++j) {
                    vertices[j][0] = x + (vertices[j][0] - x) * scale_x;
                    if (flip_y) {
                        vertices[j][1] = (2.0f * y - vertices[j][1]) + CHAR_HEIGHT;
                    }
                }

                memcpy(dst, V1, 16); dst += 4;
                memcpy(dst, V2, 16); dst += 4;
                memcpy(dst, V3, 16); dst += 4;
                memcpy(dst, V1, 16); dst += 4;
                memcpy(dst, V3, 16); dst += 4;
                memcpy(dst, V4, 16); dst += 4;

                total_vertices += 6;
            }
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, total_vertices * 16, tri_buffer);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(textVAO);
            glDrawArrays(GL_TRIANGLES, 0, total_vertices);
            glBindVertexArray(0);
        }
        void PEGLDraw_Button(const PEGLButton& button, const PEGLShaderProgram& uiShader, const PEGLShaderProgram& textShader, const glm::mat4& orthoMatrix) {
            PEGLUIQuad buttonQuad = button;
            if (button.isHovered) {
                buttonQuad.color = glm::vec3(0.5f, 0.5f, 0.5f);
            }
            else {
                buttonQuad.color = button.color;
            }
            PEGLDraw_Quad_2D(buttonQuad, uiShader, orthoMatrix);

            glUseProgram(textShader.programID);
            GLint orthoLoc = glGetUniformLocation(textShader.programID, "orthoMatrix");
            GLint colorLoc = glGetUniformLocation(textShader.programID, "textColor");

            if (orthoLoc != -1) glUniformMatrix4fv(orthoLoc, 1, GL_FALSE, &orthoMatrix[0][0]);
            if (colorLoc != -1) glUniform3f(colorLoc, button.textColor.x, button.textColor.y, button.textColor.z);

            float textY = button.position.y + button.size.y / 2.0f - 8.0f;
            float textWidth = PEGLprint_string_get_width(button.text.c_str(), 1.0f);
            float textX = button.position.x + (button.size.x - textWidth) / 2.0f;

            PEGLprint_string(textX, textY, button.text.c_str(),
                button.textColor.x, button.textColor.y, button.textColor.z,
                1.0f, false);

            glUseProgram(0);
        }
        void PEGLDraw_Checkbox(const PEGLCheckbox& checkbox, const PEGLShaderProgram& uiShader, const PEGLShaderProgram& textShader, const glm::mat4& orthoMatrix) {
            PEGLUIQuad boxQuad;
            boxQuad.mesh = checkbox.mesh;
            boxQuad.position = glm::vec2(checkbox.position.x, checkbox.position.y + (checkbox.size.y - checkbox.boxSize.y) / 2.0f);
            boxQuad.size = checkbox.boxSize;
            boxQuad.color = glm::vec3(0.1f, 0.1f, 0.1f);
            boxQuad.layer = checkbox.layer;
            PEGLDraw_Quad_2D(boxQuad, uiShader, orthoMatrix);

            if (checkbox.isChecked) {
                PEGLUIQuad innerQuad = boxQuad;
                float padding = 4.0f;
                innerQuad.position += padding;
                innerQuad.size -= 2 * padding;
                innerQuad.color = glm::vec3(0.0f, 0.8f, 0.0f);
                innerQuad.layer += 1.0f;
                PEGLDraw_Quad_2D(innerQuad, uiShader, orthoMatrix);
            }

            glUseProgram(textShader.programID);

            GLint orthoLoc = glGetUniformLocation(textShader.programID, "orthoMatrix");
            GLint colorLoc = glGetUniformLocation(textShader.programID, "textColor");

            if (orthoLoc != -1) glUniformMatrix4fv(orthoLoc, 1, GL_FALSE, &orthoMatrix[0][0]);

            glm::vec3 textColor{ 1.0f, 1.0f, 1.0f };
            if (colorLoc != -1) glUniform3f(colorLoc, textColor.x, textColor.y, textColor.z);

            float textY = checkbox.position.y + checkbox.size.y / 2.0f - 8.0f;
            float textX = checkbox.position.x + checkbox.boxSize.x + 10.0f;

            PEGLprint_string(textX, textY, checkbox.text.c_str(),
                textColor.x, textColor.y, textColor.z,
                1.0f, false);

            glUseProgram(0);
        }
        void PEGLDraw_InputField(const PEGLInputField& input, const PEGLShaderProgram& uiShader, const PEGLShaderProgram& textShader, const glm::mat4& orthoMatrix) {
            PEGLUIQuad inputQuad = input;
            inputQuad.color = glm::vec3(0.9f, 0.9f, 0.9f);
            if (input.isActive) {
                inputQuad.color = glm::vec3(0.7f, 0.8f, 0.9f);
            }
            PEGLDraw_Quad_2D(inputQuad, uiShader, orthoMatrix);

            glUseProgram(textShader.programID);

            GLint orthoLoc = glGetUniformLocation(textShader.programID, "orthoMatrix");
            GLint colorLoc = glGetUniformLocation(textShader.programID, "textColor");

            if (orthoLoc != -1) glUniformMatrix4fv(orthoLoc, 1, GL_FALSE, &orthoMatrix[0][0]);

            const std::string* display_text;
            glm::vec3 text_color;

            if (input.text.empty()) {
                display_text = &input.hintText;
                text_color = glm::vec3(0.5f, 0.5f, 0.5f);
            }
            else {
                display_text = &input.text;
                text_color = input.textColor;
            }

            if (colorLoc != -1) glUniform3f(colorLoc, text_color.x, text_color.y, text_color.z);

            float textX = input.position.x + 5.0f;
            float textY = input.position.y + input.size.y / 2.0f - 8.0f;

            std::string final_text = *display_text;

            if (input.isActive) {
                final_text += "|";
            }

            PEGLprint_string(textX, textY, final_text.c_str(),
                text_color.x, text_color.y, text_color.z,
                1.0f, false);

            glUseProgram(0);
        }
        bool PEGLis_point_in_quad(float x, float y, const PEGLUIQuad& quad) {
            return x >= quad.position.x && x <= (quad.position.x + quad.size.x) &&
                y >= quad.position.y && y <= (quad.position.y + quad.size.y);
        }


    }
}