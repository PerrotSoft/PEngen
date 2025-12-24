#pragma once
#include <windows.h>
#include <string>

namespace Input {

    class InputManager {
    public:
        static void PEIKeyPress(WORD vkCode);
        static void PEIKeyDown(WORD vkCode);
        static void PEIKeyUp(WORD vkCode);
        static void PEIMouseClickL();
        static void PEIMouseClickR();
        static void PEIMouseMoveAbsolute(int x, int y);
        static void PEIMouseMoveRelative(int dx, int dy);
        static void PEIMouseScroll(int delta);

    private:
        static void MouseEvent(DWORD flags, DWORD data = 0);
    };

}