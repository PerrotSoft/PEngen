#version 330 core
out vec4 FragColor;

in vec2 TexCoord; // Входные текстурные координаты (интерполированы)

uniform sampler2D ourTextureSampler; // Сэмплер текстуры (привязан к текстурному юниту 0)
uniform int useTexture;              // Флаг: 1, если есть текстура; 0, если нет
uniform vec3 baseColor;              // Базовый цвет, если текстура не используется

void main()
{
    if (useTexture == 1) {
        // Если флаг установлен, берем цвет из текстуры по координатам TexCoord
        FragColor = texture(ourTextureSampler, TexCoord);
    } else {
        // Иначе используем простой цвет
        FragColor = vec4(baseColor, 1.0);
    }
}