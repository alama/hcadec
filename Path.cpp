// Standard library
#include <string>
#include <sstream>
#include <cstdint>

// This class/namespace
#include "Path.h"

using namespace std;

namespace Path
{
	const std::string Filename(const std::string& path, const bool getExt, const char delim)
	{
		int start = 0;
		int end = (path.length() - 1);

		for (int i = end; i >= 0; i--)
		{
			if (path[i] == delim)
			{
				start = ++i;
				break;
			}
		}

		if (!getExt)
		{
			for (int i = start; i <= end; i++)
			{
				if (path[i] == '.')
				{
					end = --i;
					break;
				}
			}
		}

		return path.substr(start, (end > 0) ? (++end - start) : end);
	}

	const std::string Extension(const std::string& path, const bool getDot, const char delim)
	{
		int start = 0;
		int end = (path.length() - 1);

		for (int i = end; i >= 0; i--)
		{
			if (path[i] == delim)
			{
				start = (getDot) ? i : ++i;
				break;
			}
		}

		return path.substr(start, (end > 0) ? (++end - start) : end);
	}

	const std::string Directory(const std::string& path, const uint32_t cutDir, const bool addSuffix, const char delim)
	{
		int start = 0;
		int end = (path.length() - 1);
		uint32_t cut = 0;

		for (int i = end; i >= 0; i--)
		{
			if (path[i] == delim)
			{
				if (cut >= cutDir)
				{
					end = (addSuffix) ? i : --i;
					break;
				}
				++cut;
			}
			else if (i == 0)
				end = i;
		}
		
		return path.substr(start, (end > 0) ? (++end - start) : end);
	}
}