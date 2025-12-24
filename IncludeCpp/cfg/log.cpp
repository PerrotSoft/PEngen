#include "../../Include/cfg/log.h"
#include <fstream>
#include <chrono>
#include <ctime>
using namespace std::chrono;

void PElogger(const char* message)
{
	std::ofstream logFile("PEngen\\log\\log.txt", std::ios::app);
	if (logFile.is_open())
	{
		auto now = system_clock::now();
		std::time_t now_time = system_clock::to_time_t(now);
		char buffer[26];
		ctime_s(buffer, sizeof(buffer), &now_time);
		buffer[strcspn(buffer, "\n")] = '\0';
		logFile << buffer << " : " << message << std::endl;
		logFile.close();
	}
}
void PElogger_up(const char* message)
{
	std::ofstream logFile("PEngen\\log\\log_update.txt", std::ios::app);
	if (logFile.is_open())
	{
		auto now = system_clock::now();
		std::time_t now_time = system_clock::to_time_t(now);
		char buffer[26];
		ctime_s(buffer, sizeof(buffer), &now_time);
		buffer[strcspn(buffer, "\n")] = '\0';
		logFile << buffer << " : " << message << std::endl;
		logFile.close();
	}
}