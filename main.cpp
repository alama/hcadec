﻿
// HCA → WAV
// hcadec -d -o "出力ファイル名" "入力ファイル名"

// WAV → HCA (未実装) (Not yet implemented)
// hcadec -o "出力ファイル名" "入力ファイル名"

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <string.h>
#endif
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include "clHCA.h"
#include "Path.h"

//--------------------------------------------------
//	~~~~~~~~~~~~~SCREW THE RULES~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
using namespace std;
bool isList = false;
bool deleteSource = false;
inline const bool StringCompare(const char* str1, const char* str2) { return (strcmp(str1, str2) == 0); }

unsigned int ciphKey1 = 0x30DBE1AB;
unsigned int ciphKey2 = 0xCC554639;


//--------------------------------------------------
// 文字列を16進数とみなして数値に変換
//--------------------------------------------------
int atoi16(const char *s)
{
	int r = 0;
	while (*s)
	{
		if (*s >= '0' && *s <= '9')
		{
			r <<= 4;
			r |= *s - '0';
		}
		else if (*s >= 'A' && *s <= 'F')
		{
			r <<= 4;
			r |= *s - 'A' + 10;
		}
		else if (*s >= 'a' && *s <= 'f')
		{
			r <<= 4;
			r |= *s - 'a' + 10;
		}
		else
		{
			break;
		}
		s++;
	}
	return r;
}

//--------------------------------------------------
// HCA → WAV
//--------------------------------------------------
bool HCAtoWAV(char *filenameIn, char *filenameOut, unsigned int ciphKey1, unsigned int ciphKey2)
{

	// HCAファイルをデコード
	clHCA hca(ciphKey1, ciphKey2);

	if (!hca.Decode(filenameIn, filenameOut))
		return false;

	return true;
}

void Decode(const std::string& filenameIn, const std::string& filenameOut, bool list)
{
	if (list)
	{
		ifstream listFile(filenameIn);

		if (listFile.is_open())
		{
			for (string s; getline(listFile, s);)
				Decode(s, filenameOut, false);

			listFile.close();
		}
		else
			return;

	}
	else
	{
		// 入力チェック
		if (filenameIn.empty())
		{
			//wprintf(L"Error: 入力ファイルを指定してください。");
			cout << "Error: Invalid input filename." << endl;
			system("pause");
			return;
		}

		std::string fileOut;

		// デフォルト出力ファイル名
		if (filenameOut.empty())
			fileOut = Path::Directory(filenameIn) + Path::Filename(filenameIn, false) + ".wav";
		else
			fileOut = filenameOut;

		cout << "Decoding: " << Path::Filename(filenameIn) << " as " << Path::Filename(fileOut) << endl;

		// デコード
		if (!HCAtoWAV((char*)filenameIn.c_str(), (char*)fileOut.c_str(), ciphKey1, ciphKey2))
		{
			//wprintf(L"Error: HCAファイルのデコードに失敗しました。");
			cout << "Error: HCA file decode has failed." << endl << endl;
			return;
		}
		else if (deleteSource)
		{
			cout << "Deleting " << filenameIn << "..." << endl;
			remove(filenameIn.c_str());
		}
		cout << endl;
	}
}

//--------------------------------------------------
// メイン
//--------------------------------------------------
int main(int argc, char* argv[])
{
	int result = 0;

	// This stuff speeds up std::cout by quite a bit
	if (setvbuf(stdout, 0, _IOLBF, 4096) != 0)
		abort();
	if (setvbuf(stderr, 0, _IOLBF, 4096) != 0)
		abort();

	// コマンドライン解析
	std::string filenameIn;
	std::string filenameOut;

	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (StringCompare(argv[i], "--out") || StringCompare(argv[i], "-o"))
			{
				filenameOut = argv[++i];
				continue;
			}
			else if (StringCompare(argv[i], "--list") || StringCompare(argv[i], "-l"))
			{
				isList = !isList;
				cout << "List mode " << ((isList) ? "enabled." : "disabled.") << endl;
				continue;
			}
			else if (StringCompare(argv[i], "--delete") || StringCompare(argv[i], "-d"))
			{
				deleteSource = !deleteSource;
				cout << "Delete mode " << ((deleteSource) ? "enabled." : "disabled.") << endl;
				continue;
			}
			else if (StringCompare(argv[i], "--ciphA"), StringCompare(argv[i], "-a"))
			{
				ciphKey1 = atoi16(argv[++i]);
				continue;
			}
			else if (StringCompare(argv[i], "--ciphB"), StringCompare(argv[i], "-b"))
			{
				ciphKey2 = atoi16(argv[++i]);
				continue;
			}
			else
			{
				filenameIn = argv[i];
				Decode(filenameIn, filenameOut, isList);
				filenameOut = "";
			}
		}
	}
	else
	{
		cout << "Usage:" << endl;
		cout << '\t' << Path::Filename(argv[0]) << " [parameters] file [parameters] file2 [...]" << endl;

		cout << "\nParameters:" << endl;
		cout << "\t-o"
			<< "\tSets a custom output filename for the next file."
			<< endl;
		cout << "\t-d"
			<< "\tDelete the source file after successful conversion."
			<< endl;
		cout << "\t-l"
			<< "\tTreats the next file as a list of files."
			<< endl;

		system("pause");
		return result;
	}

#ifdef _DEBUG
	system("pause");
#endif
	return result;
}
