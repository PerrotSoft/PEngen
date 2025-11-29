#pragma once
#include <string>
#include <map>
#include <thread>
#include <conio.h>
#include <atomic>
#include <windows.h> // Для CP_UTF8, WideCharToMultiByte и MCI

namespace Audio {

    class AudioManager {
    public:
        // Воспроизвести одиночный звук
        static void PlaySound(const std::string& filePath, int id);

        // Воспроизвести звук в цикле
        static void PlayLoopedSound(const std::string& filePath, int id);

        // Остановить звук по ID
        static void StopSound(int id);

        // Остановить все звуки
        static void StopAll();

        // Установить громкость 0.0f..1.0f
        static void SetVolume(float volume, int id);

        // Добавить перегрузку PlaySoundW для поддержки широких строк
        static void PlaySoundW(const std::wstring& filePath, int id);

    private:
        static std::map<int, std::string> activeSounds;
        static std::map<int, std::thread> soundThreads;
        static std::map<int, std::atomic<bool>> loopFlags;

        static bool FileExists(const std::string& path);
        static bool FileExistsW(const std::wstring& path);
        static std::string getAlias(int id);

        // Для зацикленных звуков (с циклом опроса)
        static void loopThreadFunc(int id, const std::string& filePath);

        // Для одиночных звуков (с detach)
        static void singleSoundThreadFunc(int id, const std::string& filePath);
    };

} // namespace Audio