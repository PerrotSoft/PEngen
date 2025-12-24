#ifndef PEngen_H
#define PEngen_H

#define Texture unsigned int
#include <string>
#include <vector>
#include <map>
#include "../gnu/OpenGL.h"

struct GLFWwindow;

namespace PEngen {
    class Vec {
    public:
        struct Vec2 {
            float x, y;
            Vec2() : x(0.0f), y(0.0f) {}
            Vec2(float _x, float _y) : x(_x), y(_y) {}
        };

        struct Vec3 {
            float x, y, z;
            Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
            Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
            Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
        };

        struct Mat4 {
            float data[16];
            Mat4();
        };
    };
    struct Camera {
        Vec::Vec3 pos{ 0.0f, 0.0f, 5.0f };
        Vec::Vec3 front{ 0.0f, 0.0f, -1.0f };
        Vec::Vec3 up{ 0.0f, 1.0f, 0.0f };
        Vec::Vec3 right{ 1.0f, 0.0f, 0.0f };
        float yaw = -90.0f, pitch = 0.0f, roll = 0.0f;
        float farPlane = 10.0f;
    };
    class Scene {
    public:
        struct Object {
            long long id = 0;
            std::string name;
            Vec::Vec3 pos;
            Vec::Vec3 rotator;
            Vec::Vec3 size{ 1.0f, 1.0f, 1.0f };
            std::string parent_name;

            // 0=Off, 1=3D, 2=3D+Tex, 3=Button, 4=Checkbox, 5=Input, 6=Panel, 7=Image
            uint8_t state = 0;
            gnu::PEGLModel model;
            unsigned int textureID = 0;
            gnu::UI::PEGLButton button_ui;
            gnu::UI::PEGLCheckbox checkbox_ui;
            gnu::UI::PEGLInputField inputfield_ui;
            gnu::UI::PEGLPanel panel_ui;
            gnu::UI::PEGLImage image_ui;
            void (*on_click)() = nullptr; 
            bool is_pressed = false;      
            float visual_scale = 1.0f;
        };
        Camera mainCamera;
        std::vector<Object> Scene_objects;
        int screenWidth = 1280;
        int screenHeight = 720;
        unsigned int next_object_id = 1;
        std::map<std::string, gnu::PEGLShaderProgram> sceneShaders;

        Scene(std::map<std::string, gnu::PEGLShaderProgram> shaders);
        Object& AddObject(const Object& object);
        Object& SearchObject(const char* name); 
        void SetCamera(Vec::Vec3 position, float yaw, float pitch, float roll = 0.0f);
        void UpdateUI(GLFWwindow* window);
        void Render();
    };

    class Engine {
    public:
        Engine(int width, int height, const std::string& title);
        ~Engine();

        bool ShouldClose() const;
        void Update();

        Scene* GetScene() { return scene; }

    private:
        GLFWwindow* window = nullptr;
        Scene* scene = nullptr;
        std::map<std::string, gnu::PEGLShaderProgram> defaultShaders;
        void InitShaders();
    };
}
#endif