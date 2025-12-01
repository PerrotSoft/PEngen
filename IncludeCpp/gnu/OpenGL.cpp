#include "../../Include/gnu/OpenGL.h"

#define P_ENGEN_GLM_STUB_ACTIVE
#include <glad/glad.h>   // Основные функции OpenGL
#include <GLFW/glfw3.h>  // Управление окном и контекстом
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
// -------------------------------------------------------------------
// --- ИМИТАЦИЯ ЗАГРУЗКИ (СТУБЫ) ---
// -------------------------------------------------------------------

struct ImageData {
    int width = 0;
    int height = 0;
    int components = 0;
    std::vector<unsigned char> data;
};

// Функция-заглушка: имитирует чтение и декодирование PNG/JPG
ImageData load_image_data_stub(const std::string& filePath) {
    std::cout << "DEBUG: Imitating loading texture file: " << filePath << std::endl;

    // Имитация загруженного изображения (1x1 белый пиксель, 3 компонента - RGB)
    ImageData img;
    img.width = 1;
    img.height = 1;
    img.components = 3;
    img.data = { 255, 255, 255 }; // Белый пиксель

    // В реальном проекте, используйте stbi_load:
    // unsigned char *data = stbi_load(filePath.c_str(), &img.width, &img.height, &img.components, 0);
    // ...
    return img;
}

// -------------------------------------------------------------------
// --- РЕАЛИЗАЦИЯ ФУНКЦИЙ GLM/MESH ---
// -------------------------------------------------------------------

glm::mat4 gnu::Mesh::get_transform() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    // ✅ ИСПРАВЛЕНИЕ: Используйте glm::toMat4 или glm::mat4_cast для преобразования quat в mat4. 
    // Вместо model *= glm::mat4(rotation) используйте явное умножение.
    model = model * glm::toMat4(rotation);
    model = glm::scale(model, scale);
    return model;
}
glm::mat4 gnu::LightSource::get_transform() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = model * glm::toMat4(rotation);
    model = glm::scale(model, scale);
    return model;
}
void gnu::Mesh::translate(const glm::vec3& offset) {
    position += offset;
}

void gnu::Mesh::rotate(const glm::quat& delta_rotation) {
    rotation = delta_rotation * rotation;
}

void gnu::Mesh::set_scale(const glm::vec3& new_scale) {
    scale = new_scale;
}

void gnu::Mesh::delete_gpu_resources() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
    indexCount = 0;
}


// -------------------------------------------------------------------
// --- 1. ИНИЦИАЛИЗАЦИЯ И ШЕЙДЕРЫ ---
// -------------------------------------------------------------------

// Вспомогательная функция для чтения файла
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

// Компиляция отдельного шейдера
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

gnu::ShaderProgram gnu::Compile_and_Link_Shader(const std::string& vertexPath,
    const std::string& fragmentPath)
{
    ShaderProgram program;
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
            logger((std::string("Shader Linking Failed: ") + std::string(errorMessage.begin(), errorMessage.end())).c_str());
            throw std::runtime_error("Shader Linking Failed: " + std::string(errorMessage.begin(), errorMessage.end()));
        }

        glDetachShader(program.programID, vertexID);
        glDetachShader(program.programID, fragmentID);
        glDeleteShader(vertexID);
        glDeleteShader(fragmentID);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Shader Error: " << e.what() << std::endl;
        logger((std::string("Shader Error: ") + e.what()).c_str());
        program.programID = 0;
    }
    return program;
}

// -------------------------------------------------------------------
// --- 2. ОКНО И КОНТЕКСТ ---
// -------------------------------------------------------------------

// Обработчик ошибок GLFW
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// Обработчик изменения размера окна
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
gnu::ShaderProgram* gnu::Audo_Compile_and_Link_Shader() {
    gnu::ShaderProgram modelShader = gnu::Compile_and_Link_Shader("simple.vert", "simple.frag");

    gnu::ShaderProgram uiShader = gnu::Compile_and_Link_Shader("simple.vert", "simple.frag");

    gnu::ShaderProgram textShader = gnu::Compile_and_Link_Shader("text.vert", "text.frag");
	return new gnu::ShaderProgram[3]{ modelShader, uiShader, textShader };
}

void gnu::Show_Loading_Screen(GLFWwindow* window,
    const gnu::ShaderProgram& uiShader,
    const glm::mat4& orthoMatrix)
{
    if (!window || uiShader.programID == 0) {
        logger("ERROR: Failed to initialize loading screen (Window or Shader is NULL).");
        return;
    }

    // 1. ЗАГРУЗКА ТЕКСТУРЫ
    // Предполагаем, что путь ведет к вашему файлу в папке textures/
    GLuint splashTextureID = Load_Texture_From_File(BASE_PATH_ICON_PE);

    if (splashTextureID == 0) {
        logger((std::string("WARNING: Failed to load splash screen texture: ") + BASE_PATH_ICON_PE).c_str());
        // Продолжаем без заставки
        return;
    }

    // 2. ПОДГОТОВКА ОБЪЕКТА UI
    // Создаем полноэкранный квад (размер 800x600 - замените на реальные размеры окна)
    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

    // * UIQuad должен быть объявлен в gnu::UI::, и иметь публичный конструктор *
    // Мы создадим временную структуру, чтобы не зависеть от скрытой реализации
    gnu::UI::Image splashImage;

    // Создаем базовый квад (предполагаем, что вам нужен квад для UI)
    // ВАЖНО: Ваша система UI использует gnu::UI::Mesh Create_Quad_Mesh();
    // Мы должны вызвать ее, чтобы получить VAO/VBO.
    static gnu::Mesh staticQuadMesh = gnu::UI::Create_Quad_Mesh();

    splashImage.mesh = staticQuadMesh;
    splashImage.textureID = splashTextureID;
    splashImage.position = glm::vec2(0.0f, 0.0f); // Начинаем с левого верхнего угла
    splashImage.size = glm::vec2((float)windowWidth, (float)windowHeight); // Размер окна
    splashImage.layer = 1.0f; // Отображение поверх всего

    // 3. ЦИКЛ ОТОБРАЖЕНИЯ (Один кадр)

    // Очистка экрана
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Черный фон
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Отрисовка заставки
    gnu::UI::Draw_Image(splashImage, uiShader, orthoMatrix);

    // Обмен буферов, чтобы показать заставку
    glfwSwapBuffers(window);
    glfwPollEvents(); // Обработка событий, чтобы окно не зависло

    // 4. ОЧИСТКА РЕСУРСОВ ТЕКСТУРЫ (если она нужна только для заставки)
    // Если вы планируете использовать текстуру splashImage.textureID в дальнейшем, 
    // не удаляйте ее! Если нет, то:
    // glDeleteTextures(1, &splashTextureID);
}
GLFWwindow* gnu::Init_OpenGL_Window(int width, int height, const std::string& title) {
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        logger("Failed to initialize GLFW");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Для macOS

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!window) {
        std::cerr << "Failed to open GLFW window." << std::endl;
        logger("Failed to open GLFW window.");
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    // Инициализация GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        logger("Failed to initialize GLAD");
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    // Установка колбэка
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;
}

// -------------------------------------------------------------------
// --- 3. ТЕКСТУРЫ ---
GLuint gnu::Load_Texture_From_File(const std::string& filePath) {
    GLuint textureID = 0;
    int width, height, comp;

    // stbi_load использует C-строки, поэтому нужно преобразование.
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &comp, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (comp == 4) format = GL_RGBA;
        if (comp == 1) format = GL_RED;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Параметры текстуры
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Загрузка данных в GPU
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data); // Освобождаем память STB
    }
    else {
        std::cerr << "ERROR: STB_IMAGE failed to load texture: " << filePath << std::endl;
		logger(("ERROR: STB_IMAGE failed to load texture: " + filePath).c_str());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}
// -------------------------------------------------------------------
// --- 4. МЕШИ И МОДЕЛИ ---
// -------------------------------------------------------------------

void gnu::Prepare_Mesh_For_GPU(Mesh& mesh,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices)
{
    // Очистка на всякий случай
    mesh.delete_gpu_resources();

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Layout 0: Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Layout 1: Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Layout 2: TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    glBindVertexArray(0);

    mesh.indexCount = (uint32_t)indices.size();
}

gnu::Model gnu::Load_Model_From_File_OBJ(const std::string& filePath, const std::string& baseDir) {
    Model model;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err, warn;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), baseDir.c_str());

    if (!warn.empty()) {
        std::cout << "tinyobjloader WARNING: " << warn << std::endl;
		logger(("tinyobjloader WARNING: " + warn).c_str());
    }
    if (!err.empty()) {
        throw std::runtime_error("tinyobjloader ERROR: " + err);
		logger(("tinyobjloader ERROR: " + err).c_str());
    }
    if (!ret) {
        throw std::runtime_error("Failed to load/parse OBJ file: " + filePath);
		logger(("Failed to load/parse OBJ file: " + filePath).c_str());
    }

    std::map<tinyobj::index_t, uint32_t, tinyobj_index_cmp> unique_indices;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Предполагаем, что OBJ состоит из одной сетки для простоты
    // Реальный код должен обрабатывать все shapes и materials
    for (const auto& shape : shapes) {
        unique_indices.clear();
        vertices.clear();
        indices.clear();

        for (const auto& index : shape.mesh.indices) {
            if (unique_indices.count(index) == 0) {
                // Добавить новую вершину
                Vertex vertex{};
                // Position
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                // Normal
                if (index.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                }
                // TexCoords
                if (index.texcoord_index >= 0) {
                    vertex.texCoords = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Инверсия Y для OpenGL
                    };
                }

                uint32_t current_index = (uint32_t)vertices.size();
                unique_indices[index] = current_index;
                indices.push_back(current_index);
                vertices.push_back(vertex);

            }
            else {
                // Использовать существующий индекс
                indices.push_back(unique_indices[index]);
            }
        }

        // Создать и подготовить Mesh
        Mesh mesh;
        Prepare_Mesh_For_GPU(mesh, vertices, indices);
        model.meshes.push_back(std::move(mesh));
    }

    return model;
}

gnu::Model gnu::Create_Cube_Model() {
    Model model;
    std::vector<Vertex> vertices = {
        // ... (данные вершин куба) ...
        // Передняя грань
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        // Задняя грань
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        // Левая грань
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        // Правая грань
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        // Верхняя грань
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        // Нижняя грань
        {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 0, 2, 3,       // Передняя
        4, 5, 6, 4, 6, 7,       // Задняя
        8, 9, 10, 8, 10, 11,    // Левая
        12, 13, 14, 12, 14, 15, // Правая
        16, 17, 18, 16, 18, 19, // Верхняя
        20, 21, 22, 20, 22, 23  // Нижняя
    };

    Mesh cubeMesh;
    Prepare_Mesh_For_GPU(cubeMesh, vertices, indices);
    model.meshes.push_back(std::move(cubeMesh));
    return model;
}

gnu::LightSource gnu::Create_Light_Source(const glm::vec3& position, const glm::vec3& color, float intensity) {
    gnu::LightSource ls;
    ls.position = position;
    ls.color = color;
    ls.intensity = intensity;
    return ls;
}
bool gnu::tinyobj_index_cmp::operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const {
    if (a.vertex_index != b.vertex_index) return a.vertex_index < b.vertex_index;
    if (a.normal_index != b.normal_index) return a.normal_index < b.normal_index;
    // В оригинальном коде была ошибка: требуется сравнить все 3 индекса.
    return a.texcoord_index < b.texcoord_index;
}
// -------------------------------------------------------------------
// --- 5. ОТРЕНДЕРИТЬ ---
// -------------------------------------------------------------------

void gnu::Draw_Mesh(const Mesh& mesh, const ShaderProgram& shader, const glm::mat4& viewProjection) {
    if (mesh.VAO == 0) return;

    glUseProgram(shader.programID);

    // 1. МАТРИЦЫ
    glm::mat4 model = mesh.get_transform();
    glm::mat4 MVP = viewProjection * model;

    GLint MVPLoc = glGetUniformLocation(shader.programID, "MVP");
    GLint modelLoc = glGetUniformLocation(shader.programID, "model");
    GLint viewProjectionLoc = glGetUniformLocation(shader.programID, "viewProjection");

    if (MVPLoc != -1) glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, &MVP[0][0]);
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    if (viewProjectionLoc != -1) glUniformMatrix4fv(viewProjectionLoc, 1, GL_FALSE, &viewProjection[0][0]);


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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void gnu::Draw_Model(const Model& model, const ShaderProgram& shader, const glm::mat4& viewProjection) {
    for (const auto& mesh : model.meshes) {
        Draw_Mesh(mesh, shader, viewProjection);
    }
}


// -------------------------------------------------------------------
// --- 6. РЕАЛИЗАЦИЯ UI ФУНКЦИЙ (gnu::UI::...) ---
// -------------------------------------------------------------------

namespace gnu {
    // --- Фрагмент файла: OpenGL.cpp (внутри namespace gnu) ---

    namespace UI {

        // ⭐️ СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ ДЛЯ ТЕКСТА (Core Profile) ⭐️
        // FIX: Все объявлено здесь для устранения ошибок "идентификатор не определен"
        static GLuint textVAO = 0;
        static GLuint textVBO = 0;
        static GLuint textEBO = 0; // FIX: Теперь определен
        static char textBuffer[90000]; // ПРИМЕЧАНИЕ: Этот буфер статически выделен в сегменте данных, а не в стеке.
        static std::vector<uint32_t> textIndices; // FIX: Теперь определен

        // FIX: MAX_CHARS теперь определен как константа в пространстве имен
        static constexpr uint32_t MAX_CHARS = sizeof(textBuffer) / 16 / 4;


        // ----------------------------------------------------------------
        // 1. UIQuad::get_transform() 
        // ----------------------------------------------------------------
        glm::mat4 UIQuad::get_transform() const {
            float z_position = -this->layer * 0.001f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(position.x, position.y, z_position));
            model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

            return model;
        }

        // ----------------------------------------------------------------
        // 2. Create_Quad_Mesh()
        // ----------------------------------------------------------------
        Mesh Create_Quad_Mesh() {
            std::vector<Vertex> vertices = {
                {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
            };
            std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

            Mesh quadMesh;

            // ИМИТАЦИЯ Prepare_Mesh_For_GPU:
            glGenVertexArrays(1, &quadMesh.VAO);
            glGenBuffers(1, &quadMesh.VBO);
            glGenBuffers(1, &quadMesh.EBO);
            glBindVertexArray(quadMesh.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadMesh.VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadMesh.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            quadMesh.indexCount = indices.size();
            return quadMesh;
        }

        // ----------------------------------------------------------------
        // 3. Draw_Quad_2D (ОСНОВНАЯ ФУНКЦИЯ ОТРИСОВКИ)
        // ----------------------------------------------------------------
        void Draw_Quad_2D(const UIQuad& quad, const ShaderProgram& shader, const glm::mat4& orthoMatrix) {
            if (quad.mesh.VAO == 0 || shader.programID == 0) return;

            glUseProgram(shader.programID);

            // 1. МАТРИЦЫ
            glm::mat4 model = quad.get_transform();
            glm::mat4 MVP = orthoMatrix * model;

            GLint MVPLoc = glGetUniformLocation(shader.programID, "MVP");
            if (MVPLoc != -1) glUniformMatrix4fv(MVPLoc, 1, GL_FALSE, &MVP[0][0]);

            // 2. ТЕКСТУРА И ЦВЕТ
            GLint useTexLoc = glGetUniformLocation(shader.programID, "useTexture");
            GLint colorLoc = glGetUniformLocation(shader.programID, "baseColor");

            if (quad.mesh.textureID != 0) {
                if (useTexLoc != -1) glUniform1i(useTexLoc, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, quad.mesh.textureID);
                GLint samplerLoc = glGetUniformLocation(shader.programID, "ourTextureSampler");
                if (samplerLoc != -1) glUniform1i(samplerLoc, 0);
            }
            else {
                if (useTexLoc != -1) glUniform1i(useTexLoc, 0);
            }

            if (colorLoc != -1) {
                glUniform3f(colorLoc, quad.color.x, quad.color.y, quad.color.z);
            }

            // 3. Рисование
            glBindVertexArray(quad.mesh.VAO);
            glDrawElements(GL_TRIANGLES, quad.mesh.indexCount, GL_UNSIGNED_INT, 0);

            // 4. Очистка
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        }


        // ----------------------------------------------------------------
        // 4. Draw_Panel 
        // ----------------------------------------------------------------
        void Draw_Panel(const Panel& panel, const ShaderProgram& uiShader, const glm::mat4& orthoMatrix) {
            UIQuad tempQuad = panel;
            tempQuad.mesh.textureID = 0;
            Draw_Quad_2D(tempQuad, uiShader, orthoMatrix);
        }


        // ----------------------------------------------------------------
        // 5. Draw_Image
        // ----------------------------------------------------------------
        void Draw_Image(const Image& image, const ShaderProgram& uiShader, const glm::mat4& orthoMatrix) {
            if (image.textureID == 0) return;

            UIQuad tempQuad = image;
            tempQuad.mesh.textureID = image.textureID;
            tempQuad.color = glm::vec3(1.0f, 1.0f, 1.0f);

            Draw_Quad_2D(tempQuad, uiShader, orthoMatrix);
        }


        // ----------------------------------------------------------------
        // 6. print_string_get_width (Ширина текста)
        // ----------------------------------------------------------------
        float print_string_get_width(const char* text, float scale_x) {
            return stb_easy_font_width((char*)text) * scale_x;
        }

        // ----------------------------------------------------------------
        // 7. print_string (Отрисовка текста - Core Profile FIX)
        // ----------------------------------------------------------------
        void print_string(float x, float y, const char* text, float r, float g, float b,
            float scale_x, bool flip_y)
        {
            // Буфер для квадов (4 вершины * 4 float * 4 байта)
            static char quad_buffer[99999];
            // Буфер для треугольников (6 вершин * 4 float * 4 байта)
            static char tri_buffer[150000];

            static GLuint textVAO = 0;
            static GLuint textVBO = 0;
            // Используем константу по умолчанию stb_easy_font
            const float CHAR_HEIGHT = 16.0f;

            // 1. Инициализация VAO/VBO (один раз)
            if (textVAO == 0) {
                glGenVertexArrays(1, &textVAO);
                glGenBuffers(1, &textVBO);
                glBindVertexArray(textVAO);
                glBindBuffer(GL_ARRAY_BUFFER, textVBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(tri_buffer), NULL, GL_DYNAMIC_DRAW);
                // 4 float, 16 байт на вершину: x, y, z, w (мы используем только x, y, z)
                glEnableVertexAttribArray(0);
                // Последний параметр (stride) должен быть 4 * sizeof(float) = 16
                glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16, (void*)0);
                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            // 2. Генерация данных в квад-формате
            int num_quads = stb_easy_font_print(x, y, (char*)text, NULL, quad_buffer, sizeof(quad_buffer));

            if (num_quads == 0) return;

            // 3. ТРАНСФОРМАЦИЯ, МАСШТАБИРОВАНИЕ и ПЕРЕВОРОТ
            float* src = (float*)quad_buffer;
            float* dst = (float*)tri_buffer;
            int total_vertices = 0;

            for (int i = 0; i < num_quads; ++i) {
                // Чтение 4-х вершин квада (16 float)
                float V1[4], V2[4], V3[4], V4[4];

                // V1 (x, y, z, w)
                V1[0] = *src++; V1[1] = *src++; V1[2] = *src++; V1[3] = *src++;
                // V2
                V2[0] = *src++; V2[1] = *src++; V2[2] = *src++; V2[3] = *src++;
                // V3
                V3[0] = *src++; V3[1] = *src++; V3[2] = *src++; V3[3] = *src++;
                // V4
                V4[0] = *src++; V4[1] = *src++; V4[2] = *src++; V4[3] = *src++;

                // ПРИМЕНЕНИЕ МАСШТАБА (X) И ПЕРЕВОРОТА (Y)
                float* vertices[] = { V1, V2, V3, V4 };
                for (int j = 0; j < 4; ++j) {
                    // Масштабирование по X:
                    vertices[j][0] = x + (vertices[j][0] - x) * scale_x;

                    // ИСПРАВЛЕННЫЙ ПЕРЕВОРОТ по Y (Y=0 сверху)
                    if (flip_y) {
                        // Формула: Y_отраженное = (2 * Y_start - Y_original) + Высота_символа
                        vertices[j][1] = (2.0f * y - vertices[j][1]) + CHAR_HEIGHT;
                    }
                }

                // Создаем ТРЕУГОЛЬНИК 1: V1, V2, V3
                memcpy(dst, V1, 16); dst += 4;
                memcpy(dst, V2, 16); dst += 4;
                memcpy(dst, V3, 16); dst += 4;

                // Создаем ТРЕУГОЛЬНИК 2: V1, V3, V4 
                memcpy(dst, V1, 16); dst += 4;
                memcpy(dst, V3, 16); dst += 4;
                memcpy(dst, V4, 16); dst += 4;

                total_vertices += 6;
            }

            // 4. Загрузка и Отрисовка
            glBindBuffer(GL_ARRAY_BUFFER, textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, total_vertices * 16, tri_buffer);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // 5. Рисование
            glBindVertexArray(textVAO);
            // Установка цвета (уже сделана в main)
            glDrawArrays(GL_TRIANGLES, 0, total_vertices);
            glBindVertexArray(0);
        }


        // ----------------------------------------------------------------
        // 8. Draw_Button (Реализация)
        // ----------------------------------------------------------------
        void Draw_Button(const Button& button, const ShaderProgram& uiShader, const ShaderProgram& textShader, const glm::mat4& orthoMatrix) {
            UIQuad buttonQuad = button;
            if (button.isHovered) {
                buttonQuad.color = glm::vec3(0.5f, 0.5f, 0.5f);
            }
            else {
                buttonQuad.color = button.color;
            }
            Draw_Quad_2D(buttonQuad, uiShader, orthoMatrix);

            glUseProgram(textShader.programID);
            GLint orthoLoc = glGetUniformLocation(textShader.programID, "orthoMatrix");
            GLint colorLoc = glGetUniformLocation(textShader.programID, "textColor");

            if (orthoLoc != -1) glUniformMatrix4fv(orthoLoc, 1, GL_FALSE, &orthoMatrix[0][0]);
            if (colorLoc != -1) glUniform3f(colorLoc, button.textColor.x, button.textColor.y, button.textColor.z);

            float textY = button.position.y + button.size.y / 2.0f - 8.0f;
            float textWidth = print_string_get_width(button.text.c_str(), 1.0f);
            float textX = button.position.x + (button.size.x - textWidth) / 2.0f;

            print_string(textX, textY, button.text.c_str(),
                button.textColor.x, button.textColor.y, button.textColor.z,
                1.0f, false);

            glUseProgram(0);
        }


        // ----------------------------------------------------------------
        // 9. Draw_Checkbox (Реализация)
        // ----------------------------------------------------------------
        void Draw_Checkbox(const Checkbox& checkbox, const ShaderProgram& uiShader, const ShaderProgram& textShader, const glm::mat4& orthoMatrix) {
            UIQuad boxQuad;
            boxQuad.mesh = checkbox.mesh;
            boxQuad.position = glm::vec2(checkbox.position.x, checkbox.position.y + (checkbox.size.y - checkbox.boxSize.y) / 2.0f);
            boxQuad.size = checkbox.boxSize;
            boxQuad.color = glm::vec3(0.1f, 0.1f, 0.1f);
            boxQuad.layer = checkbox.layer;
            Draw_Quad_2D(boxQuad, uiShader, orthoMatrix);

            if (checkbox.isChecked) {
                UIQuad innerQuad = boxQuad;
                float padding = 4.0f;
                innerQuad.position += padding;
                innerQuad.size -= 2 * padding;
                innerQuad.color = glm::vec3(0.0f, 0.8f, 0.0f);
                innerQuad.layer += 1.0f;
                Draw_Quad_2D(innerQuad, uiShader, orthoMatrix);
            }

            glUseProgram(textShader.programID);

            GLint orthoLoc = glGetUniformLocation(textShader.programID, "orthoMatrix");
            GLint colorLoc = glGetUniformLocation(textShader.programID, "textColor");

            if (orthoLoc != -1) glUniformMatrix4fv(orthoLoc, 1, GL_FALSE, &orthoMatrix[0][0]);

            glm::vec3 textColor{ 1.0f, 1.0f, 1.0f };
            if (colorLoc != -1) glUniform3f(colorLoc, textColor.x, textColor.y, textColor.z);

            float textY = checkbox.position.y + checkbox.size.y / 2.0f - 8.0f;
            float textX = checkbox.position.x + checkbox.boxSize.x + 10.0f;

            print_string(textX, textY, checkbox.text.c_str(),
                textColor.x, textColor.y, textColor.z,
                1.0f, false);

            glUseProgram(0);
        }

        // ----------------------------------------------------------------
        // 10. Draw_InputField (Реализация)
        // ----------------------------------------------------------------
        void Draw_InputField(const InputField& input, const ShaderProgram& uiShader, const ShaderProgram& textShader, const glm::mat4& orthoMatrix) {
            UIQuad inputQuad = input;
            inputQuad.color = glm::vec3(0.9f, 0.9f, 0.9f);
            if (input.isActive) {
                inputQuad.color = glm::vec3(0.7f, 0.8f, 0.9f);
            }
            Draw_Quad_2D(inputQuad, uiShader, orthoMatrix);

            glUseProgram(textShader.programID);

            GLint orthoLoc = glGetUniformLocation(textShader.programID, "orthoMatrix");
            GLint colorLoc = glGetUniformLocation(textShader.programID, "textColor");

            if (orthoLoc != -1) glUniformMatrix4fv(orthoLoc, 1, GL_FALSE, &orthoMatrix[0][0]);

            const std::string* display_text;
            glm::vec3 text_color;

            if (input.currentText.empty()) {
                display_text = &input.hintText;
                text_color = glm::vec3(0.5f, 0.5f, 0.5f);
            }
            else {
                display_text = &input.currentText;
                text_color = input.textColor;
            }

            if (colorLoc != -1) glUniform3f(colorLoc, text_color.x, text_color.y, text_color.z);

            float textX = input.position.x + 5.0f;
            float textY = input.position.y + input.size.y / 2.0f - 8.0f;

            std::string final_text = *display_text;

            if (input.isActive) {
                final_text += "|";
            }

            print_string(textX, textY, final_text.c_str(),
                text_color.x, text_color.y, text_color.z,
                1.0f, false);

            glUseProgram(0);
        }


        // ----------------------------------------------------------------
        // 11. is_point_in_quad (Обработка кликов)
        // ----------------------------------------------------------------
        bool is_point_in_quad(float x, float y, const UIQuad& quad) {
            return x >= quad.position.x && x <= (quad.position.x + quad.size.x) &&
                y >= quad.position.y && y <= (quad.position.y + quad.size.y);
        }


    } // end namespace UI
    // ... (Остальная часть OpenGL.cpp)
} // namespace gnu::UI