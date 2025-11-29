#pragma once
#include <windows.h>
#include <string>

namespace Input {

    class InputManager {
    public:
        // --- Клавиатура ---

        /**
         * @brief Нажимает и отпускает клавишу (симуляция полного нажатия).
         * @param vkCode Код виртуальной клавиши (например, VK_SPACE, 'A').
         */
        static void KeyPress(WORD vkCode);

        /**
         * @brief Удерживает клавишу (только нажатие).
         * @param vkCode Код виртуальной клавиши.
         */
        static void KeyDown(WORD vkCode);

        /**
         * @brief Отпускает клавишу (только отпускание).
         * @param vkCode Код виртуальной клавиши.
         */
        static void KeyUp(WORD vkCode);

        // --- Мышь ---

        /**
         * @brief Выполняет клик левой кнопкой мыши.
         */
        static void MouseClickL();

        /**
         * @brief Выполняет клик правой кнопкой мыши.
         */
        static void MouseClickR();

        /**
         * @brief Перемещает курсор мыши в абсолютные координаты.
         * @param x Координата X (от 0 до 65535, масштабируется к разрешению).
         * @param y Координата Y (от 0 до 65535, масштабируется к разрешению).
         */
        static void MouseMoveAbsolute(int x, int y);

        /**
         * @brief Перемещает курсор мыши относительно текущего положения.
         * @param dx Смещение по X.
         * @param dy Смещение по Y.
         */
        static void MouseMoveRelative(int dx, int dy);

        /**
         * @brief Выполняет прокрутку колесом мыши.
         * @param delta Положительное число для прокрутки вверх, отрицательное для вниз (обычно 120 или -120).
         */
        static void MouseScroll(int delta);

    private:
        // Вспомогательная функция для отправки команды мыши
        static void MouseEvent(DWORD flags, DWORD data = 0);
    };

} // namespace Input