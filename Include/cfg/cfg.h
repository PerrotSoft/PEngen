#pragma once
#include <string>
using namespace std;

class cfg
{
	public:
		string PEload(const string& filePath);
		void PEsave(const string& filePath, const string& data);
};
