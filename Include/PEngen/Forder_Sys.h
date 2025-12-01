#pragma once

// Включаем вашу библиотеку логгера
#include "../cfg/log.h" 
#include <string>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <iostream>

// Используем std::filesystem::
namespace fs = std::filesystem;

// ==============================================================================
// 1. КОНСТАНТЫ ПУТЕЙ
// ==============================================================================
// BASE_PATH должна быть пустой, если PEngen/ является текущей рабочей директорией
const std::string BASE_PATH = "";

const std::string CFG_PATH = BASE_PATH + "cfg/";
const std::string DATA_PATH = BASE_PATH + "data/";
const std::string LOG_PATH = BASE_PATH + "log/";
const std::string MODELS_PATH = BASE_PATH + "models/";
const std::string SHADERS_PATH = BASE_PATH + "shaders/";
const std::string TEXTURES_PATH = BASE_PATH + "textures/";

const std::string DATA_ICONS_PATH = DATA_PATH + "icons/";
const std::string DATA_RE_PATH = DATA_PATH + "re/";
const std::string RECOVERY_FILE_PATH = DATA_RE_PATH + "recovery file.zip";


// ==============================================================================
// 2. ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (Объявления)
// ==============================================================================

// Предполагается, что logger объявлен в "./Include/cfg/log.h"
// extern void logger(const char* message);

bool exists_file(const std::string& path);
bool exists_folder(const std::string& path);
bool create_folder(const std::string& path);

// ==============================================================================
// 3. ФУНКЦИИ СИСТЕМЫ ПАПОК (Объявления)
// ==============================================================================

bool InitFolderStructure();
bool RestoreStructure();
void init_struct();

// ==============================================================================
// 4. ФУНКЦИЯ РАСПАКОВКИ (Объявление)
// ==============================================================================
// ТРЕБУЕТ РЕАЛЬНОЙ РЕАЛИЗАЦИИ ZIP
bool extract_zip(const std::string& zipPath, const std::string& destinationPath);