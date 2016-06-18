#pragma once

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include <stdio.h>

#ifdef __GNUC__
#define ATTRPACK __attribute__((packed, ms_struct))
#endif

#ifdef _MSC_VER
#define PACKED( __Declaration__ )\
	__pragma( pack(push, 1) ) \
	__Declaration__ \
	__pragma( pack(pop) )
#else
#define PACKED(x) x
#endif

//--------------------------------------------------
// HCA(High Compression Audio)クラス
//--------------------------------------------------
class clHCA
{
public:
	clHCA(unsigned int ciphKey1, unsigned int ciphKey2);

	// チェック
	static bool CheckFile(void* data);

	// デコード
	bool Decode(const char* filename, const char* filenameWAV, float volume = 1);
	bool Decode(FILE* fp, void* data, int size);
	bool Decode2(FILE* fp, FILE* fpHCA, int size);
	bool Decode(FILE * fp, void * data, int size, unsigned int address, FILE * fp_hca);
	bool Decode(FILE* fp, void* data, int size, unsigned int address);


private:

PACKED(
	// ファイル情報
	struct stHeader
	{
		unsigned int signature;        // 'HCA'|0x00808080
		unsigned char version;         // バージョン(1)
		unsigned char revision;        // リビジョン(3)
		unsigned short dataOffset;     // データオフセット
	}  ATTRPACK;
)
	static_assert(sizeof(stHeader) == 8, "stHeader size is not correct");

	// フォーマット情報
PACKED(
	struct stFormat
	{
		unsigned int fmt;              // 'fmt'|0x00808080
		unsigned int channelCount : 8;   // チャンネル数 1～16
		unsigned int samplingRate : 24;  // サンプリングレート 1～0x7FFFFF
		unsigned int blockCount;       // ブロック数
		unsigned short r14;            // 不明(0xC80)
		unsigned short r16;            // 不明(0x226)
	} ATTRPACK;
)
	static_assert(sizeof(stFormat) == 16, "stFormat size is not correct");

	// デコード情報
PACKED(
	struct stDecode
	{
		unsigned int dec;              // 'dec'|0x00808080
		unsigned short blockSize;      // ブロックサイズ(CBRのときに有効？) 8～0xFFFF、0のときはVBR
		unsigned char r1E;             // 不明(1) 0以上
		unsigned char r1F;             // 不明(15) r1E～0x1F
		unsigned char count1;          // type0とtype1のcount
		unsigned char count2;          // type2のcount
		unsigned char r22;             // 不明(0)
		unsigned char r23;             // 不明(0)
	} ATTRPACK;
)
	static_assert(sizeof(stDecode) == 12, "stDecode size is not correct");

PACKED(
	struct stComp
	{
		unsigned int comp;              // 'comp'|0x00808080
		unsigned short blockSize;
		unsigned char v8; //r1E (same as dec)
		unsigned char v7; //r1F (same as dec)
		unsigned char count1;
		unsigned char count2;
		unsigned char v9; //unknown
		unsigned char v10; 
		unsigned char v11;
		unsigned char v12;
		unsigned char unk00[2]; 
	} ATTRPACK;
)
	static_assert(sizeof(stComp) == 16, "stComp size is not correct");

	// 可変ビットレート情報
PACKED(
	struct stVBR
	{
		unsigned int vbr;              // 'vbr'|0x00808080
		unsigned short r04;            // 不明 0～0x1FF
		unsigned short r06;            // 不明
	} ATTRPACK;
)
	static_assert(sizeof(stVBR) == 8, "stVBR size is not correct");

	// ATHテーブル情報
PACKED(
	struct stATH
	{
		unsigned int ath;              // 'ath'|0x00808080
		unsigned short type;           // テーブルの種類(0:全て0 1:テーブル1)
	} ATTRPACK;
)
	static_assert(sizeof(stATH) == 6, "stATH size is not correct");

	// ループ情報
PACKED(
	struct stLoop
	{
		unsigned int loop;             // 'loop'|0x80808080
		unsigned int loopStart;        // ループ開始ブロックインデックス 0以上
		unsigned int loopEnd;          // ループ終了ブロックインデックス 0以上 loopStart～stFormat::blockCount-1
		unsigned short r0C;            // 不明(0x80)
		unsigned short r0E;            // 不明(0x226)
	} ATTRPACK;
)
	static_assert(sizeof(stLoop) == 16, "stLoop size is not correct");

	// CIPHテーブル情報(ブロックデータの暗号化テーブル情報)
PACKED(
	struct stCIPH
	{
		unsigned int ciph;             // 'ciph'|0x80808080
		unsigned short type;           // テーブルの種類(0:暗号化なし 1:テーブル1 56:テーブル2)
	} ATTRPACK;
)
	static_assert(sizeof(stCIPH) == 6, "stCIPH size is not correct");

	// 相対ボリューム調節情報
PACKED(
	struct stRVA
	{
		unsigned int rva;              // 'rva'|0x00808080
		float volume;                  // ボリューム
	}  ATTRPACK;
)
	static_assert(sizeof(stRVA) == 8, "stRVA size is not correct");

	// コメント情報
PACKED(
	struct stComment
	{
		unsigned int comm;             // 'comm'|0x80808080
		unsigned char r04;             // 不明 文字列の長さ？
		// char 文字列[];
	}  ATTRPACK;
)
	static_assert(sizeof(stComment) == 5, "stComment size is not correct");

	// パディング？
PACKED(
	struct stPAD
	{
		unsigned int pad;              // 'pad'|0x00808080
		// ※サイズ不明
	} ATTRPACK;
)
	static_assert(sizeof(stPAD) == 4, "stPAD size is not correct");

	unsigned int _version;
	unsigned int _revision;
	unsigned int _dataOffset;
	unsigned int _channelCount;
	unsigned int _samplingRate;
	unsigned int _blockCount;
	unsigned int _fmt_r14;
	unsigned int _fmt_r16;
	unsigned int _blockSize;
	unsigned int _dec_r1E;
	unsigned int _dec_r1F;
	unsigned int _dec_count1;
	unsigned int _dec_count2;
	unsigned int _dec_r22;
	unsigned int _dec_r23;
	unsigned int _vbr_r04;
	unsigned int _vbr_r06;
	unsigned int _ath_type;
	unsigned int _loopStart;
	unsigned int _loopEnd;
	unsigned int _loop_r0C;
	unsigned int _loop_r0E;
	unsigned int _ciph_type;
	float _rva_volume;

	class clATH
	{
	public:
		clATH();
		bool Init(int type, unsigned int key);
		unsigned char* GetTable(void);

	private:
		unsigned char _table[0x80];
		void Init0(void);
		void Init1(unsigned int key);
	} _ath;

	class clCIPH
	{
	public:
		clCIPH();
		bool Init(int type, unsigned int key1, unsigned int key2);
		void Mask(void* data, int size);

	private:
		unsigned char _table[0x100];
		void Init0(void);
		void Init1(void);
		void Init56(unsigned int key1, unsigned int key2);
		void Init56_CreateTable(unsigned char* table, unsigned char key);
	} _ciph;

	class clData
	{
	public:
		clData(void* data, int size);
		int CheckBit(int bitSize);
		int GetBit(int bitSize);
		void AddBit(int bitSize);

	private:
		unsigned char* _data;
		int _size;
		int _bit;
	};

	bool InitDecode(int channelCount, int a, int b, int count1, int count2, int e, int f);
	void Decode(clData* data);

	struct stChannel
	{
		float wave[8][0x80];
		float base[0x80];
		float block[0x80];
		float wav1[0x80];
		float wav2[0x80];
		float wav3[0x80];
		unsigned char value[0x80];
		unsigned char scale[0x80];
		unsigned char value2[8];
		int type;
		int count;
		void Decode1(clData* data);
		void Decode2(int a, int b, unsigned char* ath);
		void Decode3(void);
		void Decode4(clData* data);
		void Decode5(int index);
		void Decode6(void);
		void Decode7(void);
		void Decode8(int index);
	} _channel[0x10];

	unsigned int _ciph_key1;
	unsigned int _ciph_key2;

	static unsigned short CheckSum(void* data, int size, unsigned short sum = 0);
};
