#include "../../Include/PEngen/Scene.h"
#include "../../Include/cfg/log.h"
#include <vector>
#include <cstring>
#include <stdexcept>

namespace PEngen {

    static std::vector<PESGameObject*> Scene_objects;
    static GLuint next_object_id = 1;
    void PESInit_Scene(PESScene& scene) {
        next_object_id = scene.next_object_id;
        Scene_objects = scene.Scene_objects;
    }
    PESGameObject& PESAddObject(PESGameObject& object_template)
    {
        object_template.id = next_object_id++;
        Scene_objects.push_back(&object_template);
        return object_template;
    }
    PESGameObject& PESSearchObject(const char* name)
    {
        if (name == nullptr) {
            throw std::invalid_argument("PESSearchObject: name == nullptr");
        }
        for (auto* obj : Scene_objects)
        {
            if (obj == nullptr) continue;
            if (std::strcmp(obj->name.c_str(), name) == 0)
                return *obj;
        }
        throw std::runtime_error(std::string("Object not found: ") + name);
    }
    void PESRemoveObject(const char* name)
    {
        if (name == nullptr) {
            PElogger("PESRemoveObject: name == nullptr");
            return;
        }
        for (size_t i = 0; i < Scene_objects.size(); ++i)
        {
            if (std::strcmp(Scene_objects[i]->name.c_str(), name) == 0)
            {
                Scene_objects.erase(Scene_objects.begin() + i);
                return;
            }
        }
        PElogger((std::string("Object not found: ") + name).c_str());
    }

    glm::mat4 PESGameObject::calculate_local_matrix() const {
        glm::mat4 pos = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rot = glm::mat4_cast(rotation);
        glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
        return pos * rot * sca;
    }
    vector<PESRenderer::PESRObject> PESRenderer::render_pre()
    {
        vector<PESRObject> render_queue;
        render_queue.clear();
        render_queue.reserve(Scene_objects.size());

        for (size_t i = 0; i < Scene_objects.size(); ++i)
        {
            PESGameObject* obj = Scene_objects[i];
            PESRObject ro;
            ro.id = obj->id;
            if (!obj->parent_name.empty())
            {
                PESGameObject& parent = PESSearchObject(obj->parent_name.c_str());
                ro.position = parent.position + obj->position;
                ro.rotation = parent.rotation * obj->rotation;
            }
            else
            {
                ro.position = obj->position;
                ro.rotation = obj->rotation;
            }
            ro.scale = obj->scale;
            ro.state = obj->state;
            ro.cached_transform = obj->calculate_local_matrix();
            ro.model = obj->model;
            ro.button_ui = obj->button_ui;
            ro.checkbox_ui = obj->checkbox_ui;
            ro.inputfield_ui = obj->inputfield_ui;
            ro.panel_ui = obj->panel_ui;
            ro.image_ui = obj->image_ui;

            render_queue.push_back(ro);
        }
        return render_queue;
    }

    void PESRenderer::render()
    {
        auto render_queue = render_pre();
        if (render_queue.empty()) return;
        static gnu::PEGLShaderProgram* s_shaders = nullptr;
        static glm::mat4 orthoMatrix = glm::mat4(1.0f);
        static bool shaders_initialized = false;
        if (!shaders_initialized) {
            s_shaders = gnu::PEGLAudo_Compile_and_Link_Shader();
            orthoMatrix = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f);
            shaders_initialized = true;
        }

        gnu::PEGLShaderProgram modelShader = s_shaders ? s_shaders[0] : gnu::PEGLShaderProgram{};
        gnu::PEGLShaderProgram uiShader = s_shaders ? s_shaders[1] : gnu::PEGLShaderProgram{};
        gnu::PEGLShaderProgram textShader = s_shaders ? s_shaders[2] : gnu::PEGLShaderProgram{};
        glm::mat4 viewProjection = glm::mat4(1.0f);
        for (auto& ro : render_queue)
        {
            if (ro.state == 0) continue;
            if (ro.state == 1 || ro.state == 2)
            {
                for (auto& mesh : ro.model.meshes) {
                    mesh.position += ro.position;
                }
                gnu::PEGLDraw_Model(ro.model, modelShader, viewProjection);
            }
            else
            {
                switch (ro.state)
                {
                case 3:
                    gnu::UI::PEGLDraw_Button(ro.button_ui, uiShader, textShader, orthoMatrix);
                    break;
                case 4:
                    gnu::UI::PEGLDraw_Checkbox(ro.checkbox_ui, uiShader, textShader, orthoMatrix);
                    break;
                case 5:
                    gnu::UI::PEGLDraw_InputField(ro.inputfield_ui, uiShader, textShader, orthoMatrix);
                    break;
                case 6:
                    gnu::UI::PEGLDraw_Panel(ro.panel_ui, uiShader, orthoMatrix);
                    break;
                case 7:
                    gnu::UI::PEGLDraw_Image(ro.image_ui, uiShader, orthoMatrix);
                    break;
                default:
                    break;
                }
            }
        }
    }
}