#pragma once
#include <string>
#include <map>
#include <thread>
#include <conio.h>
#include <atomic>
#include <windows.h>

namespace Audio {

    class AudioManager {
    public:
        static void PEAPlaySound(const std::string& filePath, int id);
        static void PEAPlayLoopedSound(const std::string& filePath, int id);
        static void PEAStopSound(int id);
        static void PEAStopAll();
        static void PEASetVolume(float volume, int id);
        static void PEAPlaySoundW(const std::wstring& filePath, int id);

    private:
        static std::map<int, std::string> activeSounds;
        static std::map<int, std::thread> soundThreads;
        static std::map<int, std::atomic<bool>> loopFlags;

        static bool FileExists(const std::string& path);
        static bool FileExistsW(const std::wstring& path);
        static std::string getAlias(int id);
        static void loopThreadFunc(int id, const std::string& filePath);
        static void singleSoundThreadFunc(int id, const std::string& filePath);
    };

}