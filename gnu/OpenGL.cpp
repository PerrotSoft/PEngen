#include "OpenGL.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <map> // Используется для кэширования индексов
// --- КОСТЫЛЬ ДЛЯ ИМИТАЦИИ ДЕКОДЕРА ИЗОБРАЖЕНИЙ ---
// В реальном проекте, данные image_data, width, height, comp будут получены
// при помощи библиотеки stb_image или аналогичной.
struct ImageData {
    int width = 0;
    int height = 0;
    int components = 0;
    std::vector<unsigned char> data;
};

// Функция-заглушка: имитирует чтение и декодирование PNG/JPG
// В реальном проекте здесь stbi_load()
ImageData load_image_data_stub(const std::string& filePath) {
    std::cout << "DEBUG: Imitating loading texture file: " << filePath << std::endl;

    // Имитация загруженного изображения (1x1 белый пиксель, 3 компонента - RGB)
    ImageData img;
    img.width = 1;
    img.height = 1;
    img.components = 3;
    img.data = { 255, 255, 255 }; // R=255, G=255, B=255 (Белый цвет)
    return img;
}
// ----------------------------------------------------


namespace gnu {
    // -------------------------------------------------------------------
    // --- РЕАЛИЗАЦИЯ МЕТОДОВ MESH (Идентична ранее предоставленной) ---
    // -------------------------------------------------------------------
    glm::mat4 Mesh::get_transform() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, scale);
        model *= glm::mat4_cast(rotation);
        model = glm::translate(model, position);
        return model;
    }
    void Mesh::translate(const glm::vec3& offset) { position += offset; }
    void Mesh::rotate(const glm::quat& delta_rotation) { rotation = delta_rotation * rotation; }
    void Mesh::set_scale(const glm::vec3& new_scale) { scale = new_scale; }
    void Mesh::delete_gpu_resources() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
        // Текстуру удалять осторожно, если она общая!
        // if (textureID) glDeleteTextures(1, &textureID); 
        VAO = VBO = EBO = 0;
        indexCount = 0;
    }

    // -------------------------------------------------------------------
    // --- 1. РЕАЛИЗАЦИЯ ИНИЦИАЛИЗАЦИИ ---
    // ------------------------------------
// --- 1. РЕАЛИЗАЦИЯ ИНИЦИАЛИЗАЦИИ ---
// ------------------------------------
    GLFWwindow* Init_OpenGL_Window(int width, int height, const std::string& title) {
        if (!glfwInit()) return nullptr;

        // Настройка версии и профиля OpenGL
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window) { glfwTerminate(); return nullptr; }

        // Установка контекста и включение VSync
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Включаем vsync

        // Инициализация GLAD (после установки контекста GLFW!)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "ERROR: Failed to initialize GLAD" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return nullptr;
        }

        glViewport(0, 0, width, height);
        return window;
    }

    // -------------------------------------------------------------------
    // --- 2. РЕАЛИЗАЦИЯ ПОДГОТОВКИ РЕСУРСОВ ---
    // ------------------------------------------

    void Prepare_Mesh_For_GPU(Mesh& mesh,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices)
    {
        mesh.indexCount = (uint32_t)indices.size();

        glGenVertexArrays(1, &mesh.VAO);
        glBindVertexArray(mesh.VAO);

        glGenBuffers(1, &mesh.VBO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &mesh.EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        // Настройка указателей атрибутов вершин
        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        glEnableVertexAttribArray(1); // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2); // TexCoords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    GLuint Load_Texture_From_File(const std::string& filePath) {
        GLuint textureID = 0;
        int width, height, nrChannels;

        // Используем stb_image для загрузки данных изображения
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = 0;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            // Добавьте это, если хотите, чтобы текстуры PNG с альфа-каналом работали:
            if (nrChannels == 4) format = GL_RGBA;

            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            // Настройки текстуры 
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Загрузка данных в GPU
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(data); // Освобождаем память C++
        }
        else {
            std::cerr << "ERROR: STB_IMAGE failed to load texture: " << filePath << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        return textureID;
    }

    // Вспомогательная функция для чтения файла
    static std::string read_file_to_string(const std::string& filePath) {
        std::ifstream fileStream(filePath);
        if (!fileStream.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }
        std::stringstream ss;
        ss << fileStream.rdbuf();
        return ss.str();
    }

    // Вспомогательная функция для компиляции одного шейдера
    static GLuint compile_single_shader(const std::string& source, GLenum type) {
        GLuint id = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        GLint result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE) {
            GLint length;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            std::vector<char> message(length);
            glGetShaderInfoLog(id, length, &length, message.data());
            std::string error_msg = (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") + std::string(" compilation error:\n") + message.data();
            std::cerr << "ERROR: " << error_msg << std::endl;
            glDeleteShader(id);
            throw std::runtime_error(error_msg);
        }
        return id;
    }

    // Компиляция и линковка шейдеров
    ShaderProgram Compile_and_Link_Shader(const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        ShaderProgram program = { 0 };

        try {
            std::string vertCode = read_file_to_string(vertexPath);
            std::string fragCode = read_file_to_string(fragmentPath);

            GLuint vertID = compile_single_shader(vertCode, GL_VERTEX_SHADER);
            GLuint fragID = compile_single_shader(fragCode, GL_FRAGMENT_SHADER);

            program.programID = glCreateProgram();
            glAttachShader(program.programID, vertID);
            glAttachShader(program.programID, fragID);
            glLinkProgram(program.programID);

            // Проверка ошибок линковки
            GLint success;
            glGetProgramiv(program.programID, GL_LINK_STATUS, &success);
            if (!success) {
                char infoLog[1024];
                glGetProgramInfoLog(program.programID, 1024, nullptr, infoLog);
                std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
                glDeleteProgram(program.programID);
                program.programID = 0;
            }

            glDeleteShader(vertID);
            glDeleteShader(fragID);

        }
        catch (std::exception& e) {
            std::cerr << "Shader setup failed: " << e.what() << std::endl;
            program.programID = 0;
        }

        return program;
    }
    // OpenGL.cpp

// ... (после остальных вспомогательных функций) ...

// -------------------------------------------------------------------
// --- РЕАЛИЗАЦИЯ ЗАГРУЗКИ МОДЕЛИ OBJ (tiny_obj_loader) ---
// -------------------------------------------------------------------
    gnu::Model Load_Model_From_File_OBJ(const std::string& filePath, const std::string& baseDir) {
        gnu::Model model;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        // 1. Загрузка данных OBJ
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), baseDir.c_str());

        if (!warn.empty()) {
            std::cout << "TinyObjLoader WARNING: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "TinyObjLoader ERROR: " << err << std::endl;
        }
        if (!ret) {
            throw std::runtime_error("Failed to load/parse OBJ file: " + filePath);
        }

        // 2. Обработка материалов (загрузка текстур)
        std::vector<GLuint> textureIDs;
        for (const auto& mat : materials) {
            GLuint textureID = 0;
            if (!mat.diffuse_texname.empty()) {
                std::string texturePath = baseDir + mat.diffuse_texname;
                textureID = Load_Texture_From_File(texturePath);
            }
            textureIDs.push_back(textureID);
        }
        if (textureIDs.empty()) {
            textureIDs.push_back(0);
        }


        // 3. Преобразование данных в структуры gnu::Model/Mesh
        for (const auto& shape : shapes) {
            gnu::Mesh mesh;
            std::vector<gnu::Vertex> vertices;
            std::vector<uint32_t> indices;

            // ?? ИСПРАВЛЕНИЕ: Используем кастомный компаратор tinyobj_index_cmp ??
            std::map<tinyobj::index_t, uint32_t, tinyobj_index_cmp> indexMap;

            for (const auto& index : shape.mesh.indices) {
                if (indexMap.count(index)) {
                    indices.push_back(indexMap.at(index));
                }
                else {
                    gnu::Vertex vertex = {};

                    // A. Позиция
                    vertex.position.x = attrib.vertices[3 * index.vertex_index + 0];
                    vertex.position.y = attrib.vertices[3 * index.vertex_index + 1];
                    vertex.position.z = attrib.vertices[3 * index.vertex_index + 2];

                    // B. Нормаль (если есть)
                    if (index.normal_index >= 0) {
                        vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
                        vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
                        vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
                    }

                    // C. Текстурные координаты (если есть)
                    if (index.texcoord_index >= 0) {
                        vertex.texCoords.x = attrib.texcoords[2 * index.texcoord_index + 0];
                        vertex.texCoords.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
                    }

                    vertices.push_back(vertex);
                    uint32_t newIndex = (uint32_t)vertices.size() - 1;
                    indices.push_back(newIndex);
                    indexMap[index] = newIndex;
                }
            }

            // 4. Назначение материала/текстуры
            int material_id = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
            if (material_id >= 0 && material_id < textureIDs.size()) {
                mesh.textureID = textureIDs[material_id];
            }
            else {
                mesh.textureID = 0;
            }

            // 5. Подготовка к GPU и добавление в модель
            if (!vertices.empty()) {
                Prepare_Mesh_For_GPU(mesh, vertices, indices);
                model.meshes.push_back(mesh);
            }
        }

        return model;
    }

    // -------------------------------------------------------------------
    // --- 3. РЕАЛИЗАЦИЯ РЕНДЕРИНГА (Отрисовка) ---
    // ---------------------------------------------

    // Отрисовка ОДНОГО самостоятельного Меша
    void Draw_Mesh(const Mesh& mesh, const ShaderProgram& shader, const glm::mat4& viewProjection) {
        if (mesh.VAO == 0 || shader.programID == 0 || mesh.indexCount == 0) return;

        glUseProgram(shader.programID);

        // 1. Отправка MVP-матрицы
        glm::mat4 modelMatrix = mesh.get_transform();
        glm::mat4 MVP = viewProjection * modelMatrix;
        GLint MVPLoc = glGetUniformLocation(shader.programID, "MVP");
        if (MVPLoc != -1) glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, &MVP[0][0]);

        // 2. ТЕКСТУРА И ЦВЕТ
        GLint useTexLoc = glGetUniformLocation(shader.programID, "useTexture");

        if (mesh.textureID != 0) {
            // Использовать текстуру
            if (useTexLoc != -1) glUniform1i(useTexLoc, 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);

            GLint samplerLoc = glGetUniformLocation(shader.programID, "ourTextureSampler");
            if (samplerLoc != -1) glUniform1i(samplerLoc, 0); // Юнит 0

        }
        else {
            // Использовать baseColor
            if (useTexLoc != -1) glUniform1i(useTexLoc, 0);

            GLint colorLoc = glGetUniformLocation(shader.programID, "baseColor");
            if (colorLoc != -1) {
                glUniform3f(colorLoc, mesh.baseColor.x, mesh.baseColor.y, mesh.baseColor.z);
            }
        }

        // 3. Рисование
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);

        // 4. Очистка
        glBindVertexArray(0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Отрисовка всей Модели
    void Draw_Model(const Model& model, const ShaderProgram& shader, const glm::mat4& viewProjection) {
        for (const auto& mesh : model.meshes) {
            Draw_Mesh(mesh, shader, viewProjection);
        }
    }

} // Закрытие namespace gnu