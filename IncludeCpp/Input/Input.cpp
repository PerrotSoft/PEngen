#include "../../Include/Input/Input.h"

namespace Input {
    void InputManager::MouseEvent(DWORD flags, DWORD data) {
        INPUT input;

        ZeroMemory(&input, sizeof(input));

        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flags;
        input.mi.mouseData = data;

        SendInput(1, &input, sizeof(INPUT));
    }
    void InputManager::PEIKeyDown(WORD vkCode) {
        INPUT input;
        ZeroMemory(&input, sizeof(input));

        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = 0;

        SendInput(1, &input, sizeof(INPUT));
    }
    void InputManager::PEIKeyUp(WORD vkCode) {
        INPUT input;
        ZeroMemory(&input, sizeof(input));

        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(1, &input, sizeof(INPUT));
    }
    void InputManager::PEIKeyPress(WORD vkCode) {
        PEIKeyDown(vkCode);
        PEIKeyUp(vkCode);
    }
    void InputManager::PEIMouseClickL() {
        MouseEvent(MOUSEEVENTF_LEFTDOWN);
        MouseEvent(MOUSEEVENTF_LEFTUP);
    }
    void InputManager::PEIMouseClickR() {
        MouseEvent(MOUSEEVENTF_RIGHTDOWN);
        MouseEvent(MOUSEEVENTF_RIGHTUP);
    }
    void InputManager::PEIMouseMoveAbsolute(int x, int y) {
        INPUT input;
        ZeroMemory(&input, sizeof(input));

        input.type = INPUT_MOUSE;
        input.mi.dx = x;
        input.mi.dy = y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

        SendInput(1, &input, sizeof(INPUT));
    }
    void InputManager::PEIMouseMoveRelative(int dx, int dy) {
        INPUT input;
        ZeroMemory(&input, sizeof(input));

        input.type = INPUT_MOUSE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;

        SendInput(1, &input, sizeof(INPUT));
    }
    void InputManager::PEIMouseScroll(int delta) {
        MouseEvent(MOUSEEVENTF_WHEEL, delta);
    }
}