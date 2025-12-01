#include "../../Include/Audio/Audio.h"
#include "../../Include/cfg/log.h"
#include <windows.h> 
#include <iostream>
#include <filesystem>
#pragma comment(lib, "Winmm.lib")

using Audio::AudioManager;

namespace Audio {

    // Статические переменные
    std::map<int, std::string> AudioManager::activeSounds;
    std::map<int, std::thread> AudioManager::soundThreads;
    std::map<int, std::atomic<bool>> AudioManager::loopFlags;

    // Проверка существования файла
    bool Audio::AudioManager::FileExists(const std::string& path) {
        return std::filesystem::exists(path);
    }
    bool Audio::AudioManager::FileExistsW(const std::wstring& path) {
        return std::filesystem::exists(path);
    }

    // Генерация alias для MCI
    std::string AudioManager::getAlias(int id) {
        return "sound" + std::to_string(id);
    }

    // --- ПОТОК ДЛЯ ОДИНОЧНОГО ВОСПРОИЗВЕДЕНИЯ ---
    void AudioManager::singleSoundThreadFunc(int id, const std::string& filePath) {
        std::string alias = getAlias(id);
        std::string command;

        // 1. Открываем файл
        command = "open \"" + filePath + "\" type mpegvideo alias " + alias;
        if (mciSendStringA(command.c_str(), NULL, 0, NULL) != 0) {
            logger(("Audio Error: File not found: " + filePath).c_str());
            return;
        }

        // 2. Воспроизводим и ждем завершения
        command = "play " + alias + " wait";
        mciSendStringA(command.c_str(), NULL, 0, NULL);

        // 3. Закрываем ресурс
        mciSendStringA(("close " + alias).c_str(), NULL, 0, NULL);
    }


    // --- ПОТОК ДЛЯ ЗАЦИКЛЕННОГО ВОСПРОИЗВЕДЕНИЯ (с циклом опроса) ---
    void AudioManager::loopThreadFunc(int id, const std::string& filePath) {
        auto it = activeSounds.find(id);
        if (it == activeSounds.end()) return;

        std::string alias = it->second;
        std::string command;

        command = "open \"" + filePath + "\" type mpegvideo alias " + alias;
        if (mciSendStringA(command.c_str(), NULL, 0, NULL) != 0) {
            std::cout << "Audio Error [Thread]: Could not open file: " << filePath << std::endl;
            logger(("Audio Error: File not found: " + filePath).c_str());
            AudioManager::loopFlags[id] = false;
            return;
        }

        char lengthStr[32] = { 0 };
        mciSendStringA(("status " + alias + " length").c_str(), lengthStr, 32, NULL);
        int lengthMs = atoi(lengthStr);
        if (lengthMs <= 0) lengthMs = 1000;

        const int POLLING_INTERVAL_MS = 100;

        while (AudioManager::loopFlags[id]) {
            mciSendStringA(("play " + alias + " from 0").c_str(), NULL, 0, NULL);

            int slept_time = 0;

            // Цикл опроса: спим короткими интервалами и проверяем флаг
            while (AudioManager::loopFlags[id] && slept_time < lengthMs) {
                Sleep(POLLING_INTERVAL_MS);
                slept_time += POLLING_INTERVAL_MS;
            }

            // Если вышли из цикла из-за флага остановки, прерываем воспроизведение
            if (!AudioManager::loopFlags[id]) {
                mciSendStringA(("stop " + alias).c_str(), NULL, 0, NULL);
                break;
            }
        }

        mciSendStringA(("close " + alias).c_str(), NULL, 0, NULL);
    }


    // --- PlaySound (ОДИНОЧНЫЙ ЗВУК) ---
    void AudioManager::PlaySound(const std::string& filePath, int id) {
        if (!FileExists(filePath)) {
            std::cout << "Audio Error: File not found: " << filePath << std::endl;
            logger(("Audio Error: File not found: " + filePath).c_str());
            return;
        }

        if (activeSounds.find(id) != activeSounds.end()) {
            StopSound(id);
        }

        std::string alias = getAlias(id);

        activeSounds[id] = alias;
        loopFlags[id] = false;

        // Запускаем поток для одиночного воспроизведения и используем detach
        std::thread t([id, filePath]() { singleSoundThreadFunc(id, filePath); });
        t.detach();
    }


    // --- PlayLoopedSound (ЗАЦИКЛЕННЫЙ ЗВУК) ---
    void AudioManager::PlayLoopedSound(const std::string& filePath, int id) {
        if (!FileExists(filePath)) {
            std::cout << "Audio Error: File not found: " << filePath << std::endl;
            logger(("Audio Error: File not found: " + filePath).c_str());
            return;
        }

        std::string alias = getAlias(id);
        StopSound(id);

        activeSounds[id] = alias;
        loopFlags[id] = true;

        // Запускаем поток (joinable)
        soundThreads[id] = std::thread([id, filePath]() { loopThreadFunc(id, filePath); });
    }

    // --- StopSound (ОСТАНОВКА) ---
    void AudioManager::StopSound(int id) {
        // 1. МГНОВЕННАЯ ТИШИНА: Отправляем команду остановки MCI
        auto soundIt = activeSounds.find(id);
        if (soundIt != activeSounds.end()) {
            std::string alias = soundIt->second;
            mciSendStringA(("stop " + alias).c_str(), NULL, 0, NULL);
        }
        else {
            logger((std::string("Audio Warning: Cannot Stop. Sound ID ") + std::to_string(id) + " not found.").c_str());
            return;
        }

        // 2. Сигнализируем потоку о завершении
        auto loopIt = loopFlags.find(id);
        if (loopIt != loopFlags.end()) {
            loopIt->second = false;
        }

        // 3. Ждем завершения потока ТОЛЬКО, если он joinable (зацикленный звук)
        auto threadIt = soundThreads.find(id);
        if (threadIt != soundThreads.end()) {
            if (threadIt->second.joinable()) {
                threadIt->second.join(); // Ждем, пока loopThreadFunc закроет MCI
            }
            else {
                // Если поток не joinable (detached PlaySound), нужно закрыть MCI,
                mciSendStringA(("close " + soundIt->second).c_str(), NULL, 0, NULL);
            }
            soundThreads.erase(threadIt);
        }
        else {
            // Если нет записи в soundThreads, это detached PlaySound. Закрываем ресурс.
            mciSendStringA(("close " + soundIt->second).c_str(), NULL, 0, NULL);
        }

        // 4. Очищаем записи
        activeSounds.erase(soundIt);
        loopFlags.erase(id);
    }

    // --- SetVolume, StopAll, PlaySoundW ---

    void AudioManager::SetVolume(float volume, int id) {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;

        DWORD vol = static_cast<DWORD>(volume * 1000);

        auto it = activeSounds.find(id);
        if (it != activeSounds.end()) {
            std::string command = "setaudio " + it->second + " volume to " + std::to_string(vol);
            mciSendStringA(command.c_str(), NULL, 0, NULL);
        }
        else {
            std::cout << "Audio Warning: Cannot set volume. Sound ID " << id << " not found." << std::endl;
            logger((std::string("Audio Warning: Cannot set volume. Sound ID ") + std::to_string(id) + " not found.").c_str());
        }
    }

    void AudioManager::StopAll() {
        for (auto& [id, flag] : loopFlags) {
            flag = false;
        }

        for (auto& [id, alias] : activeSounds) {
            mciSendStringA(("stop " + alias).c_str(), NULL, 0, NULL);
            mciSendStringA(("close " + alias).c_str(), NULL, 0, NULL);
        }

        for (auto it = soundThreads.begin(); it != soundThreads.end(); ) {
            if (it->second.joinable()) {
                it->second.join();
            }
            it = soundThreads.erase(it);
        }

        activeSounds.clear();
        loopFlags.clear();
    }
    void AudioManager::PlaySoundW(const std::wstring& filePath, int id) {
        if (!FileExistsW(filePath)) {
            std::wcout << L"Audio Error: File not found: " << filePath << std::endl;

            // Формирование сообщения для logger (const char*)
            std::string log_message = "Audio Error: File not found: name";

            // Отправка в logger
            logger(log_message.c_str());
            return;
        }

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Path(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, &utf8Path[0], size_needed, nullptr, nullptr);

        if (!utf8Path.empty() && utf8Path.back() == '\0') {
            utf8Path.pop_back();
        }

        PlaySound(utf8Path, id);
    }

} // namespace Audio