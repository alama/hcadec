#pragma once

#include <string>

namespace Path
{
	const std::string Filename(const std::string& path,		const bool getExt = true,		const char split = '\\');
	const std::string Extension(const std::string& path,	const bool getDot = true,		const char split = '\\');
	const std::string Directory(const std::string& path,	const bool addSuffix = true,	const char split = '\\');
}