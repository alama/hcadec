#pragma once

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include <stdio.h>
#include <stdint.h>

#ifdef __GNUC__
#define ATTRPACK __attribute__((packed, ms_struct))
#else
#define ATTRPACK
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
	clHCA(uint32_t ciphKey1, uint32_t ciphKey2);

	// チェック
	static bool CheckFile(void* data);

	// デコード
	bool Decode(const char* filename, const char* filenameWAV, float volume = 1);
	bool Decode(FILE* fp, void* data, int size);
	bool Decode2(FILE* fp, FILE* fpHCA, int size);
	bool Decode(FILE * fp, void * data, int size, uint32_t address, FILE * fp_hca);
	bool Decode(FILE* fp, void* data, size_t size, uint32_t address);


private:

PACKED(
	// ファイル情報
	struct stHeader
	{
		uint32_t signature;        // 'HCA'|0x00808080
		unsigned char version;         // バージョン(1)
		unsigned char revision;        // リビジョン(3)
		uint16_t dataOffset;     // データオフセット
	}  ATTRPACK;
)
	static_assert(sizeof(stHeader) == 8, "stHeader size is not correct");

	// フォーマット情報
PACKED(
	struct stFormat
	{
		uint32_t fmt;              // 'fmt'|0x00808080
		uint32_t channelCount : 8;   // チャンネル数 1～16
		uint32_t samplingRate : 24;  // サンプリングレート 1～0x7FFFFF
		uint32_t blockCount;       // ブロック数
		uint16_t r14;            // 不明(0xC80)
		uint16_t r16;            // 不明(0x226)
	} ATTRPACK;
)
	static_assert(sizeof(stFormat) == 16, "stFormat size is not correct");

	// デコード情報
PACKED(
	struct stDecode
	{
		uint32_t dec;              // 'dec'|0x00808080
		uint16_t blockSize;      // ブロックサイズ(CBRのときに有効？) 8～0xFFFF、0のときはVBR
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
		uint32_t comp;              // 'comp'|0x00808080
		uint16_t blockSize;
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
		uint32_t vbr;              // 'vbr'|0x00808080
		uint16_t r04;            // 不明 0～0x1FF
		uint16_t r06;            // 不明
	} ATTRPACK;
)
	static_assert(sizeof(stVBR) == 8, "stVBR size is not correct");

	// ATHテーブル情報
PACKED(
	struct stATH
	{
		uint32_t ath;              // 'ath'|0x00808080
		uint16_t type;           // テーブルの種類(0:全て0 1:テーブル1)
	} ATTRPACK;
)
	static_assert(sizeof(stATH) == 6, "stATH size is not correct");

	// ループ情報
PACKED(
	struct stLoop
	{
		uint32_t loop;             // 'loop'|0x80808080
		uint32_t loopStart;        // ループ開始ブロックインデックス 0以上
		uint32_t loopEnd;          // ループ終了ブロックインデックス 0以上 loopStart～stFormat::blockCount-1
		uint16_t r0C;            // 不明(0x80)
		uint16_t r0E;            // 不明(0x226)
	} ATTRPACK;
)
	static_assert(sizeof(stLoop) == 16, "stLoop size is not correct");

	// CIPHテーブル情報(ブロックデータの暗号化テーブル情報)
PACKED(
	struct stCIPH
	{
		uint32_t ciph;             // 'ciph'|0x80808080
		uint16_t type;           // テーブルの種類(0:暗号化なし 1:テーブル1 56:テーブル2)
	} ATTRPACK;
)
	static_assert(sizeof(stCIPH) == 6, "stCIPH size is not correct");

	// 相対ボリューム調節情報
PACKED(
	struct stRVA
	{
		uint32_t rva;              // 'rva'|0x00808080
		float volume;                  // ボリューム
	}  ATTRPACK;
)
	static_assert(sizeof(stRVA) == 8, "stRVA size is not correct");

	// コメント情報
PACKED(
	struct stComment
	{
		uint32_t comm;             // 'comm'|0x80808080
		unsigned char r04;             // 不明 文字列の長さ？
		// char 文字列[];
	}  ATTRPACK;
)
	static_assert(sizeof(stComment) == 5, "stComment size is not correct");

	// パディング？
PACKED(
	struct stPAD
	{
		uint32_t pad;              // 'pad'|0x00808080
		// ※サイズ不明
	} ATTRPACK;
)
	static_assert(sizeof(stPAD) == 4, "stPAD size is not correct");

	uint32_t _version;
	uint32_t _revision;
	uint32_t _dataOffset;
	uint32_t _channelCount;
	uint32_t _samplingRate;
	uint32_t _blockCount;
	uint32_t _fmt_r14;
	uint32_t _fmt_r16;
	uint32_t _blockSize;
	uint32_t _dec_r1E;
	uint32_t _dec_r1F;
	uint32_t _dec_count1;
	uint32_t _dec_count2;
	uint32_t _dec_r22;
	uint32_t _dec_r23;
	uint32_t _vbr_r04;
	uint32_t _vbr_r06;
	uint32_t _ath_type;
	uint32_t _loopStart;
	uint32_t _loopEnd;
	uint32_t _loop_r0C;
	uint32_t _loop_r0E;
	uint32_t _ciph_type;
	float _rva_volume;

	class clATH
	{
	public:
		clATH();
		bool Init(int type, uint32_t key);
		unsigned char* GetTable(void);

	private:
		unsigned char _table[0x80];
		void Init0(void);
		void Init1(uint32_t key);
	} _ath;

	class clCIPH
	{
	public:
		clCIPH();
		bool Init(int type, uint32_t key1, uint32_t key2);
		void Mask(void* data, int size);

	private:
		unsigned char _table[0x100];
		void Init0(void);
		void Init1(void);
		void Init56(uint32_t key1, uint32_t key2);
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

	uint32_t _ciph_key1;
	uint32_t _ciph_key2;

	static uint16_t CheckSum(void* data, int size, uint16_t sum = 0);
};
