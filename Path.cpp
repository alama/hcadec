#include <sstream>
#include <string>

#include "Path.h"

using namespace std;

const std::string Path::Filename(const std::string& path, const bool getExt, const char split)
{
	stringstream result; result.str("");
	int splitpos = 0;
	bool foundSplit = false;

	for (int i = (path.length() - 1); i >= 0; i--)
	{
		if (foundSplit = (path[i] == split))
		{
			splitpos = i;
			break;
		}
	}

	for (unsigned int i = (foundSplit ? (splitpos + 1) : 0); i < path.length(); i++)
	{
		if (!getExt && path[i] == '.')
			break;
		else
			result << path[i];
	}

	return result.str();
}
const std::string Path::Extension(const std::string& path, const bool getDot, const char split)
{
	stringstream result; result.str("");
	int splitpos = 0;
	string file = Filename(path, true, split);

	for (int i = (file.length() - 1); i >= 0; i--)
	{
		if (file[i] == '.')
		{
			splitpos = i;
			break;
		}
	}

	for (unsigned int i = ((getDot) ? splitpos : (splitpos + 1)); i < file.length(); i++)
		result << file[i];

	return result.str();
}
const std::string Path::Directory(const std::string& path, const bool addSuffix, const char split)
{
	stringstream result; result.str("");
	int splitpos = (path.length() - 1);
	bool foundSplit = false;

	for (int i = (path.length() - 1); i >= 0; i--)
	{
		if (foundSplit = (path[i] == split))
		{
			splitpos = i;
			break;
		}
	}

	if (foundSplit)
	{
		for (int i = 0; i < ((addSuffix) ? (splitpos + 1) : splitpos); i++)
			result << path[i];
	}

	return result.str();
}