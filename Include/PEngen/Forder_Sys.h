#pragma once
#include "../cfg/log.h" 
#include <string>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
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

bool PEexists_file(const std::string& path);
bool PEexists_folder(const std::string& path);
bool PEcreate_folder(const std::string& path);

bool PEInitFolderStructure();
bool PERestoreStructure();
void PEinit_struct();

bool PEextract_zip(const std::string& zipPath, const std::string& destinationPath);