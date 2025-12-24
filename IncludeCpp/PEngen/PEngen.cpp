#include "../../Include/PEngen/PEngen.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

namespace PEngen {
    Vec::Mat4::Mat4() {
        std::memset(data, 0, sizeof(float) * 16);
        data[0] = data[5] = data[10] = data[15] = 1.0f;
    }
    glm::mat4 CalculateGLMMatrix(const Vec::Vec3& pos, const Vec::Vec3& rot, const Vec::Vec3& sca) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
        m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1, 0, 0));
        m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0, 0, 1));
        m = glm::scale(m, glm::vec3(sca.x, sca.y, sca.z));
        return m;
    }
    Engine::Engine(int width, int height, const std::string& title) {
        window = gnu::PEGLInit_OpenGL_Window(width, height, title);
        if (!window) throw std::runtime_error("Failed to init window");
        InitShaders();
        scene = new Scene(defaultShaders);
        scene->screenWidth = width;
        scene->screenHeight = height;
    }
    Engine::~Engine() {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
        delete scene;
    }
    bool Engine::ShouldClose() const {
        return window ? glfwWindowShouldClose(window) : true;
    }
    void Engine::Update() {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (scene) scene->Render();

        if (window) {
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    void Engine::InitShaders() {
        gnu::PEGLShaderProgram* compiled = gnu::PEGLAudo_Compile_and_Link_Shader();
        if (compiled) {
            defaultShaders["model"] = compiled[0];
            defaultShaders["ui"] = compiled[1];
            defaultShaders["text"] = compiled[2];
            delete[] compiled;
        }
    }
    Scene::Scene(std::map<std::string, gnu::PEGLShaderProgram> shaders) : sceneShaders(shaders) {}
    Scene::Object& Scene::AddObject(const Object& object) {
        Scene_objects.push_back(object);
        Scene_objects.back().id = next_object_id++;
        return Scene_objects.back();
    }
    Scene::Object& Scene::SearchObject(const char* name) {
        for (auto& obj : Scene_objects) {
            if (obj.name == name) return obj;
        }
        throw std::runtime_error("Object not found");
    }
    void Scene::SetCamera(Vec::Vec3 position, float yaw, float pitch, float roll) {
        mainCamera.pos = position;
        mainCamera.yaw = yaw;
        mainCamera.pitch = pitch;
        mainCamera.roll = roll;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        glm::vec3 front = glm::normalize(direction);
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(roll), front);
        glm::vec3 up = glm::vec3(rollMat * glm::vec4(glm::cross(right, front), 0.0f));

        mainCamera.front = { front.x, front.y, front.z };
        mainCamera.right = { right.x, right.y, right.z };
        mainCamera.up = { up.x, up.y, up.z };
    }
    void Scene::UpdateUI(GLFWwindow* window) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        float mouseX = (float)x;
        float mouseY = (float)y;

        for (auto& obj : Scene_objects) {
            if (obj.state == 3) {
                bool hovered = gnu::UI::PEGLis_point_in_quad(mouseX, mouseY, obj.button_ui);
                if (hovered && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    obj.is_pressed = true;
                    obj.visual_scale = 0.9f;
                }
                else if (obj.is_pressed && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
                    obj.is_pressed = false;
                    obj.visual_scale = 1.0f;
                    if (hovered && obj.on_click) obj.on_click();
                }
                else {
                    obj.visual_scale = 1.0f;
                }
                obj.button_ui.size = glm::vec2(200.0f * obj.visual_scale, 50.0f * obj.visual_scale);
            }
            else if (obj.state == 4) {
                bool hovered = gnu::UI::PEGLis_point_in_quad(mouseX, mouseY, obj.checkbox_ui);
                if (hovered && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    obj.is_pressed = true;
                }
                else if (obj.is_pressed && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
                    obj.is_pressed = false;
                    if (hovered) {
                        obj.checkbox_ui.isChecked = !obj.checkbox_ui.isChecked;
                        std::cout << "[UI] Checkbox '" << obj.name << "' is now: " << (obj.checkbox_ui.isChecked ? "ON" : "OFF") << std::endl;
                    }
                }
            }
            else if (obj.state == 5) {
                bool hovered = gnu::UI::PEGLis_point_in_quad(mouseX, mouseY, obj.inputfield_ui);

                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    obj.inputfield_ui.isActive = hovered;
                }
                if (obj.inputfield_ui.isActive) {
                    static bool backspacePressed = false;
                    if (glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
                        if (!backspacePressed && !obj.inputfield_ui.text.empty()) {
                            obj.inputfield_ui.text.pop_back();
                            backspacePressed = true;
                        }
                    }
                    else {
                        backspacePressed = false;
                    }
                    for (int i = 32; i <= 90; i++) {
                        static bool keyStates[91] = { false };
                        if (glfwGetKey(window, i) == GLFW_PRESS) {
                            if (!keyStates[i]) {
                                obj.inputfield_ui.text += (char)i;
                                keyStates[i] = true;
                            }
                        }
                        else {
                            keyStates[i] = false;
                        }
                    }
                }
            }
        }
    }
    void Scene::Render() {
        glEnable(GL_DEPTH_TEST);
        glm::vec3 p(mainCamera.pos.x, mainCamera.pos.y, mainCamera.pos.z);
        glm::vec3 f(mainCamera.front.x, mainCamera.front.y, mainCamera.front.z);
        glm::vec3 u(mainCamera.up.x, mainCamera.up.y, mainCamera.up.z);

        glm::mat4 view = glm::lookAt(p, p + f, u);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / screenHeight, 0.1f, mainCamera.farPlane);
        glm::mat4 vp = projection * view;
        glm::mat4 ortho = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

        for (auto& obj : Scene_objects) {
            if (obj.state == 0) continue;
            if (obj.state == 1 || obj.state == 2) {
                glm::mat4 modelMatrix = CalculateGLMMatrix(obj.pos, obj.rotator, obj.size);
                if (!obj.parent_name.empty()) {
                    try {
                        Object& p = SearchObject(obj.parent_name.c_str());
                        glm::mat4 pMat = CalculateGLMMatrix(p.pos, p.rotator, p.size);
                        modelMatrix = pMat * modelMatrix;
                    }
                    catch (...) {}
                }
                if (obj.textureID > 0) {
                    for (auto& mesh : obj.model.meshes) {
                        mesh.textureID = obj.textureID;
                    }
                }

                gnu::PEGLDraw_Model(obj.model, sceneShaders["model"], vp * modelMatrix);
            }
            else if (obj.state >= 3 && obj.state <= 7) {
                glDisable(GL_DEPTH_TEST);

                switch (obj.state) {
                case 3:
                    gnu::UI::PEGLDraw_Button(obj.button_ui, sceneShaders["ui"], sceneShaders["text"], ortho);
                    break;
                case 4:
                    gnu::UI::PEGLDraw_Checkbox(obj.checkbox_ui, sceneShaders["ui"], sceneShaders["text"], ortho);
                    break;
                case 5:
                    gnu::UI::PEGLDraw_InputField(obj.inputfield_ui, sceneShaders["ui"], sceneShaders["text"], ortho);
                    break;
                case 6:
                    gnu::UI::PEGLDraw_Panel(obj.panel_ui, sceneShaders["ui"], ortho);
                    break;
                case 7:
                    if (obj.textureID > 0) {
                        obj.image_ui.textureID = obj.textureID;
                    }
                    gnu::UI::PEGLDraw_Image(obj.image_ui, sceneShaders["ui"], ortho);
                    break;
                }

                glEnable(GL_DEPTH_TEST);
            }
        }
    }
}