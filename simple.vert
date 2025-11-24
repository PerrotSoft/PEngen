#version 330 core
layout (location = 0) in vec3 aPos;      // Позиция из VAO (layout 0)
layout (location = 2) in vec2 aTexCoord; // Текстурные координаты из VAO (layout 2)

out vec2 TexCoord; // Выход в фрагментный шейдер

uniform mat4 MVP; // Матрица Model-View-Projection, переданная из C++

void main()
{
    // Умножаем позицию вершины на объединенную матрицу MVP
    gl_Position = MVP * vec4(aPos, 1.0);
    
    // Передаем текстурные координаты дальше
    TexCoord = aTexCoord;
}