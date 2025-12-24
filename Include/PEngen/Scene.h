#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <map>
#include <string>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "../gnu/OpenGL.h"

using GLuint = unsigned int;
using namespace std;
using namespace gnu;

namespace PEngen {
    struct PESGameObject {
        GLuint id = 0;
        string name;

        glm::vec3 position{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };
        glm::mat4 cached_transform = glm::mat4(1.0f);
        string parent_name;
        uint8_t state = 0;// 0 - неактивен, 1 - 3D Model Base Color, 2 - Model Texture, 3 - UI Bututton, 4 - UI Checkbox, 5 - UI InputField, 6 - UI Panel, 7 - UI Image
        PEGLModel model;
        // UI
        UI::PEGLButton button_ui;
        UI::PEGLCheckbox checkbox_ui;
        UI::PEGLInputField inputfield_ui;
        UI::PEGLPanel panel_ui;
        UI::PEGLImage image_ui;

        glm::mat4 calculate_local_matrix() const;
    };
    struct PESScene {
        string scene_name;
        std::vector<PESGameObject*> Scene_objects;
        GLuint next_object_id = 1;
    };
    struct EngineContext {
        PESScene* active_scene = nullptr;
        bool is_update_task_running = false;
        std::unique_ptr<std::thread> update_thread;
    };
    extern EngineContext GContext;
    class PESRenderer {
    public:
        struct PESRObject {
            GLuint id = 0;

            // Трансформация (Локальная)
            glm::vec3 position{ 0.0f };
            glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
            glm::vec3 scale{ 1.0f };
            glm::mat4 cached_transform = glm::mat4(1.0f);
            uint8_t state = 0;// 0 - неактивен, 1 - 3D Model Base Color, 2 - Model Texture, 3 - UI Bututton, 4 - UI Checkbox, 5 - UI InputField, 6 - UI Panel, 7 - UI Image
            PEGLModel model;
            UI::PEGLButton button_ui;
            UI::PEGLCheckbox checkbox_ui;
            UI::PEGLInputField inputfield_ui;
            UI::PEGLPanel panel_ui;
            UI::PEGLImage image_ui;
        };

        static vector<PESRenderer::PESRObject> render_pre();
        static void render();
    };

    void PESInit_Scene(PESScene& scene);

    PESGameObject& PESAddObject(PESGameObject& object_template);
    PESGameObject& PESSearchObject(const char* name);

    void PESRemoveObject(const char* name);

}