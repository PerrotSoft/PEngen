#include "cfg.h"
#include <string>
#include <fstream>
using namespace std;
void cfg::save(const string& filePath, const string& data)
{
	std::ofstream file("PEngen\\cfg\\" + filePath);
	if (file.is_open())
	{
		file << data;
		file.close();
	}
}
string cfg::load(const string& filePath)
{
	std::ifstream file("PEngen\\cfg\\"+filePath);
	string data;
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			data += line + "\n";
		}
		file.close();
	}
	return data;
}
