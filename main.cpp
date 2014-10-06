
// HCA → WAV
// hcadec -d -o "出力ファイル名" "入力ファイル名"

// WAV → HCA (未実装) (Not yet implemented)
// hcadec -o "出力ファイル名" "入力ファイル名"

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include "clHCA.h"
#include "Path.h"

//--------------------------------------------------
//	~~~~~~~~~~~~~SCREW THE RULES~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
using namespace std;

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

bool deleteSource = false;

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

	unsigned int ciphKey1 = 0x30DBE1AB;
	unsigned int ciphKey2 = 0xCC554639;

	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-' || argv[i][0] == '/')
			{
				switch (argv[i][1])
				{
				case 'o':
					if (i + 1 < argc)
						filenameOut = argv[++i];
					continue;

				case 'd':
					deleteSource = true;
					break;

				case 'a':
					if (i + 1 < argc)
						ciphKey1 = atoi16(argv[++i]);
					continue;

				case 'b':
					if (i + 1 < argc)
						ciphKey2 = atoi16(argv[++i]);
					continue;
				}
			}

			filenameIn = argv[i];

			// 入力チェック
			if (filenameIn.empty())
			{
				//wprintf(L"Error: 入力ファイルを指定してください。");
				cout << "Error: Invalid input filename." << endl;
				system("pause");
				result = -1;
			}

			// デフォルト出力ファイル名
			filenameOut = Path::Directory(filenameIn) + Path::Filename(filenameIn, false) + ".wav";

			// デコード
			if (!HCAtoWAV((char*)filenameIn.c_str(), (char*)filenameOut.c_str(), ciphKey1, ciphKey2))
			{
				//wprintf(L"Error: HCAファイルのデコードに失敗しました。");
				cout << "Error: HCA file decode has failed." << endl;
				result = -1;
			}
			else if (deleteSource)
			{
				remove(filenameIn.c_str());
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
		
		system("pause");
		return result;
	}

#ifdef _DEBUG
	system("pause");
#endif
	return result;
}
