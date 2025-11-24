#include "gnu/OpenGL.h" 
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp> 
#include <glm/gtc/constants.hpp> 
#include <iostream>
#include <vector>

// -----------------------------------------------------
// --- Вспомогательные данные для создания грани куба ---
// -----------------------------------------------------

// Создает 4 вершины для одной квадратной грани
std::vector<gnu::Vertex> create_face_vertices() {
    return {
        //   Позиция           | Нормаль       | Текстурные Координаты
        {{-1.5f, -1.5f, 1.1f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.5f, -1.5f, 1.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.5f,  1.5f, 1.1f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.5f,  1.5f, 1.1f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
    };
}
// Индексы для отрисовки двух треугольников, составляющих квадрат
std::vector<uint32_t> create_face_indices() {
    return { 0, 1, 2, 2, 3, 0 };
}

// -----------------------------------------------------
// --- ОСНОВНАЯ ЛОГИКА ---
// -----------------------------------------------------

int main() {
    using namespace gnu;

    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;
    const float PI = glm::pi<float>();
    const float offset = 0.5f;

    // --- 1. ИНИЦИАЛИЗАЦИЯ СИСТЕМЫ ---
    GLFWwindow* window = Init_OpenGL_Window(SCREEN_WIDTH, SCREEN_HEIGHT, "GNU OpenGL Engine");
    if (!window) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // --- 2. ПОДГОТОВКА РЕСУРСОВ ---
    ShaderProgram cubeShader = Compile_and_Link_Shader("simple.vert", "simple.frag");
    if (cubeShader.programID == 0) return -1;

    GLuint dirtTextureID = Load_Texture_From_File("PEngen\\textures\\viking_room.png");
    if (dirtTextureID == 0) std::cerr << "WARNING: Texture not loaded. Using solid color." << std::endl;


    // --- 3. СОЗДАНИЕ 3D-МОДЕЛИ "КУБ" ---
    Model cubeModel;
    std::vector<Vertex> faceVertices = create_face_vertices();
    std::vector<uint32_t> faceIndices = create_face_indices();

    // ⭐️ Вектор для хранения статической ориентации каждой грани (ЛОКАЛЬНО) ⭐️
    std::vector<glm::quat> face_orientations(6);

    // Создаем 6 Мешей (граней) для куба
    for (int i = 0; i < 6; ++i) {
        Mesh face;
        face.textureID = dirtTextureID;
        face.baseColor = glm::vec3(1.0f);

        Prepare_Mesh_For_GPU(face, faceVertices, faceIndices);

        // Установка начальной трансформации для размещения грани
        switch (i) {
        case 0: // +Z (Front)
            face.position = { 0, 0, offset };
            face_orientations[i] = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity
            break;
        case 1: // -Z (Back)
            face.position = { 0, 0, -offset };
            face_orientations[i] = glm::quat({ 0, PI, 0 }); // 180 deg around Y
            break;
        case 2: // +X (Right)
            face.position = { offset, 0, 0 };
            face_orientations[i] = glm::quat({ 0, PI / 2.0f, 0 }); // 90 deg around Y
            break;
        case 3: // -X (Left)
            face.position = { -offset, 0, 0 };
            face_orientations[i] = glm::quat({ 0, -PI / 2.0f, 0 }); // -90 deg around Y
            break;
        case 4: // +Y (Top)
            face.position = { 0, offset, 0 };
            face_orientations[i] = glm::quat({ -PI / 2.0f, 0, 0 }); // -90 deg around X
            break;
        case 5: // -Y (Bottom)
            face.position = { 0, -offset, 0 };
            face_orientations[i] = glm::quat({ PI / 2.0f, 0, 0 }); // 90 deg around X
            break;
        }

        // Устанавливаем статическое вращение для первого кадра
        face.rotation = face_orientations[i];

        cubeModel.meshes.push_back(face);
    }

    // --- 4. НАСТРОЙКА ВИДОВОЙ МАТРИЦЫ ---
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(3, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 viewProjection = projection * view;


    // --- 5. ОСНОВНОЙ ЦИКЛ РЕНДЕРИНГА ---
    while (!glfwWindowShouldClose(window)) {
        // Очистка кадра
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- ЛОГИКА ГЛОБАЛЬНОГО ВРАЩЕНИЯ КУБА ---
        float time = (float)glfwGetTime();
        float angle = time * glm::radians(45.0f);

        // Глобальное вращение вокруг оси Y
        glm::quat total_rotation = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));

        // Применяем вращение к КАЖДОМУ МЕШУ
        for (size_t i = 0; i < cubeModel.meshes.size(); ++i) {
            Mesh& mesh = cubeModel.meshes[i];

            // ⭐️ Объединяем ГЛОБАЛЬНОЕ вращение и ЛОКАЛЬНУЮ ориентацию
            // Локальная ориентация применяется первой (умножается справа)
            glm::quat combined_rotation = total_rotation * face_orientations[i];

            // Сохраняем итоговое вращение в mesh.rotation
            mesh.rotation = combined_rotation;
        }

        // --- 6. ОТРЕНДЕРИТЬ МОДЕЛЬ ---
        Draw_Model(cubeModel, cubeShader, viewProjection);

        // Обновление окна
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- 7. ОЧИСТКА РЕСУРСОВ ---
    for (auto& mesh : cubeModel.meshes) {
        mesh.delete_gpu_resources();
    }
    glDeleteProgram(cubeShader.programID);
    glDeleteTextures(1, &dirtTextureID);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}