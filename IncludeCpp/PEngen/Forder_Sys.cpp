#include "../../Include/PEngen/Forder_Sys.h"
#include <iostream>
#include <fstream>
#include <sstream>

// ==============================================================================
// 2. РЕАЛИЗАЦИЯ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ
// ==============================================================================

bool exists_file(const std::string& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool exists_folder(const std::string& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool create_folder(const std::string& path) {
    if (!exists_folder(path)) {
        std::error_code ec;
        // Используем create_directories, чтобы создать весь путь
        if (fs::create_directories(path, ec)) {
            logger((std::string("Folder created: ") + path).c_str());
            return true;
        }
        else {
            logger((std::string("ERROR: Failed to create folder: ") + path + " | Reason: " + ec.message()).c_str());
            return false;
        }
    }
    return true;
}

// ==============================================================================
// 3. ФУНКЦИЯ РАСПАКОВКИ (Место для вашего кода cpp-zipper)
// ==============================================================================

/**
 * @brief Распаковывает ZIP-архив в целевой каталог (РЕАЛИЗАЦИЯ).
 * @note Этот блок должен быть заменен на реальный код cpp-zipper/MiniZip.
 * @param zipPath Путь к ZIP-файлу.
 * @param destinationPath Целевой путь для распаковки (BASE_PATH).
 * @return true, если распаковка успешна.
 */
bool extract_zip(const std::string& zipPath, const std::string& destinationPath) {
    logger((std::string("Attempting to extract: ") + zipPath + " to " + destinationPath).c_str());

    // ---------------------------------------------------------------------------------
    // >>> ВСТАВЬТЕ СЮДА КОД cpp-zipper/MiniZip <<<
    // ---------------------------------------------------------------------------------

    // ВРЕМЕННЫЙ КОД: Запускает создание папок для имитации успешного восстановления, 
    // но в реальном проекте должен быть код распаковки!

    // Если реальный код распаковки отсутствует, мы завершаем программу с ошибкой.
    // Замените этот throw на код, который возвращает false при неудаче распаковки.

    // ВРЕМЕННАЯ ИМИТАЦИЯ УСПЕХА ДЛЯ ПРОДОЛЖЕНИЯ ЛОГИКИ:
    logger("ZIP library code is missing. SIMULATING SUCCESS.");
    // Имитируем, что распаковка успешно создала критические папки:
    if (!create_folder(CFG_PATH) || !create_folder(DATA_PATH) || !create_folder(LOG_PATH)) {
        return false; // Имитация сбоя при создании базовых папок
    }
    return true;
}

// ==============================================================================
// 4. РЕАЛИЗАЦИЯ ОСНОВНЫХ ФУНКЦИЙ СИСТЕМЫ ПАПОК
// ==============================================================================

bool InitFolderStructure() {
    logger("Initializing PEngen Folder Structure...");

    std::vector<std::string> folders = {
        CFG_PATH, DATA_PATH, LOG_PATH, MODELS_PATH, SHADERS_PATH, TEXTURES_PATH,
        DATA_ICONS_PATH, DATA_RE_PATH
    };

    bool all_ok = true;
    for (const auto& folder : folders) {
        if (!create_folder(folder)) {
            all_ok = false;
        }
    }

    if (!all_ok) {
        logger("CRITICAL: Failed to create some folders. Restoration will be attempted.");
    }

    return all_ok;
}

bool RestoreStructure() {
    logger("Starting Integrity Check...");

    std::vector<std::string> critical_paths = {
        CFG_PATH, DATA_PATH, LOG_PATH,
        RECOVERY_FILE_PATH
    };

    bool integrity_ok = true;

    // Проверка целостности
    for (const auto& path : critical_paths) {
        if (!exists_folder(path) && !exists_file(path)) {
            logger((std::string("MISSING CRITICAL RESOURCE: ") + path).c_str());
            integrity_ok = false;
            break;
        }
    }

    if (integrity_ok) {
        logger("Integrity check passed. Structure is intact.");
        return true;
    }

    // Если целостность нарушена, пытаемся восстановить
    logger("CRITICAL: Structure integrity FAILED! Attempting restoration...");

    if (!exists_file(RECOVERY_FILE_PATH)) {
        logger((std::string("FATAL: Recovery file not found: ") + RECOVERY_FILE_PATH).c_str());
        logger("Cannot restore project structure.");
        return false;
    }

    logger("Recovery file found. Starting ZIP extraction...");

    if (extract_zip(RECOVERY_FILE_PATH, BASE_PATH)) {
        logger("Structure successfully restored from recovery file.");
        return true;
    }
    else {
        logger("FATAL: Failed to extract recovery file. Recovery FAILED.");
        return false;
    }
}

void init_struct() {
    // 1. Пытаемся создать все папки.
    InitFolderStructure();

    // 2. Всегда вызываем RestoreStructure, чтобы проверить целостность
    if (!RestoreStructure()) {
        // Если RestoreStructure вернул false (структура повреждена и восстановление не удалось)
        throw std::runtime_error("FATAL: Project structure could not be verified or restored. Exiting.");
    }

    logger("Project structure initialized and verified successfully.");
}