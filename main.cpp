
// HCA → WAV
// hca.exe -d -o "出力ファイル名" "入力ファイル名"

// WAV → HCA (未実装)
// hca.exe -o "出力ファイル名" "入力ファイル名"

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <stdio.h>
#include "clHCA.h"
#include "Path.h"

//--------------------------------------------------
//	SCREW THE RULES
//--------------------------------------------------
bool scan = false;

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

	if (!hca.Decode(filenameIn, filenameOut, scan))
		return false;

	return true;
}

//--------------------------------------------------
// メイン
//--------------------------------------------------
int main(int argc, char* argv[])
{
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

				case 'a':
					if (i + 1 < argc)
						ciphKey1 = atoi16(argv[++i]);
					continue;

				case 'b':
					if (i + 1 < argc)
						ciphKey2 = atoi16(argv[++i]);
					continue;

				case 's':
					scan = true;
					continue;
				}
			}

			filenameIn = argv[i];

			// 入力チェック
			if (filenameIn.empty())
			{
				wprintf(L"Error: 入力ファイルを指定してください。\n");
				system("pause");
				return -1;
			}

			// デフォルト出力ファイル名
			filenameOut = Path::Directory(filenameIn) + Path::Filename(filenameIn, false) + ".wav";

			// デコード
			if (!HCAtoWAV((char*)filenameIn.c_str(), (char*)filenameOut.c_str(), ciphKey1, ciphKey2))
			{
				wprintf(L"Error: HCAファイルのデコードに失敗しました。\n");
				system("pause");
				return -1;
			}
		}
	}

	system("pause");
	return 0;
}
