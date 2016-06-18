#pragma once

#include <string>

namespace Path
{
	// Returns the filename from a path string.
	// Excludes the file extension if "getExt" is false.
	const std::string Filename(const std::string& path,		const bool getExt = true,		const char delim = '\\');
	// Returns the file extension from a path string.
	// Excludes "delim" if "getDot" is false.
	const std::string Extension(const std::string& path,	const bool getDot = true,		const char delim = '.');
	// Returns the parent directory of a directory or file, and cuts "cut" directories.
	// Excludes suffix "delim" (e.g C:\folder\file = C:\folder) if "addSuffix" is false.
	const std::string Directory(const std::string& path, const uint32_t cutDir = 0, const bool addSuffix = true, const char delim = '\\');
}