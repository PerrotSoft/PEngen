#pragma once
#include <string>
using namespace std;

class cfg
{
	public:
		string load(const string& filePath);
		void save(const string& filePath, const string& data);
};
