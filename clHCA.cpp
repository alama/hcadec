
//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include "clHCA.h"
#include <memory.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#ifdef HAVE_SNDFILE
#include <string>
#include <sndfile.h>
#endif
#if _DEBUG
#include <cassert>
#define _ASSERT 1
#else
#define _ASSERT 0
#endif

//--------------------------------------------------
// インライン関数
//--------------------------------------------------
#ifdef __GNUC__
inline  int16_t bswap(int16_t v){ return __builtin_bswap16(v); }
inline uint16_t bswap(uint16_t v){ return __builtin_bswap16(v); }
inline  int32_t bswap(int32_t v){ return __builtin_bswap32(v); }
inline uint32_t bswap(uint32_t v){return __builtin_bswap32(v); }
inline  int64_t bswap(int64_t v){ return __builtin_bswap64(v); }
inline uint64_t bswap(uint64_t v){ return __builtin_bswap64(v); }
#elif defined ( _MSC_VER)
inline  int16_t bswap(int16_t v) { return _byteswap_ushort(v); }
inline uint16_t bswap(uint16_t v) { return _byteswap_ushort(v); }
inline  int32_t bswap(int32_t v) { return _byteswap_ulong(v); }
inline uint32_t bswap(uint32_t v) { return _byteswap_ulong(v); }
inline  int64_t bswap(int64_t v) { return _byteswap_uint64(v); }
inline uint64_t bswap(uint64_t v) { return _byteswap_uint64(v); }
#else
inline  int16_t bswap(int16_t v){ int16_t r = v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; return r; }
inline uint16_t bswap(uint16_t v){ uint16_t r = v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; return r; }
inline  int32_t bswap(int32_t v){ int32_t r = v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; return r; }
inline uint32_t bswap(uint32_t v){ uint32_t r = v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; return r; }
inline  int64_t bswap(int64_t v){ int64_t r = v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; return r; }
inline uint64_t bswap(uint64_t v){ uint64_t r = v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; r <<= 8; v >>= 8; r |= v & 0xFF; return r; }
#endif

inline float bswap(float v){
		float ft = v;
		int32_t it = 0;
		memcpy(&ft, &it, sizeof (uint32_t));
		it = bswap(it);
		memcpy(&it, &ft, sizeof (float));
		return ft;
	}

//--------------------------------------------------
// コンストラクタ
//--------------------------------------------------
clHCA::clHCA(uint32_t ciphKey1, uint32_t ciphKey2) :
_ath(), _ciph(), _ciph_key1(ciphKey1), _ciph_key2(ciphKey2){}

//--------------------------------------------------
// HCAチェック
//--------------------------------------------------
bool clHCA::CheckFile(void *data){
	return (data && (*(uint32_t *)data & 0x7F7F7F7F) == 0x00414348);
}

#ifndef _WIN32
bool fopen_s(FILE** pFile, const char *filename, const char *mode)
{
	*pFile = fopen(filename, mode);
	return *pFile == NULL;
}
#endif

//--------------------------------------------------
// デコード
//--------------------------------------------------
bool clHCA::Decode(const char *filename, const char *filenameWAV, float volume){

	// チェック
	if (!(filename&&filenameWAV))return false;

	// 開く
	FILE *fp;
#ifdef HAVE_SNDFILE
	SNDFILE *fp2 = NULL;
#else
	FILE *fp2;
#endif
	if (fopen_s(&fp, filename, "rb"))return false;
#ifndef HAVE_SNDFILE
	if (fopen_s(&fp2, filenameWAV, "wb")){ fclose(fp); return false; }
#endif

	// ヘッダを解析
	uint8_t data[0x800];
	fread(data, sizeof(data), 1, fp);
	if (!Decode(fp2, data, sizeof(data), 0))
	{
#ifndef HAVE_SNDFILE
		fclose(fp2);
#endif
		fclose(fp); return false;
	}
	else
	{
#ifdef HAVE_SNDFILE
		SF_INFO si;
		si.samplerate = _samplingRate;
		si.channels = _channelCount;
		si.format = SF_FORMAT_WAV|SF_FORMAT_PCM_32;
		fp2 = sf_open(filenameWAV, SFM_WRITE, &si);
		if (!fp2)
		{
			fclose(fp); return false;
		}
#if 0
		float cl = 1.0;
		sf_command(fp2, SFC_SET_COMPRESSION_LEVEL, &cl, sizeof(cl));
#endif
#else
		// WAVEヘッダを書き込み
		struct stWAVEHeader{
			char riff[4];
			uint32_t riffSize;
			char wave[4];
			char fmt[4];
			uint32_t fmtSize;
			uint16_t fmtType;
			uint16_t fmtChannelCount;
			uint32_t fmtSamplingRate;
			uint32_t fmtSamplesPerSec;
			uint16_t fmtSamplingSize;
			uint16_t fmtBitCount;
			char data[4];
			uint32_t dataSize;
		} wav = { { 'R', 'I', 'F', 'F'}, 0, { 'W', 'A', 'V', 'E'}, {'f', 'm', 't', ' '}, 0x10, 1, 0, 0, 0, 0, 32, {'d', 'a', 't', 'a'}, 0 };
		wav.fmtChannelCount = (uint16_t)_channelCount;
		wav.fmtSamplingRate = _samplingRate;
		wav.fmtSamplingSize = wav.fmtBitCount/8 * wav.fmtChannelCount;
		wav.fmtSamplesPerSec = wav.fmtSamplingRate*wav.fmtSamplingSize;
		wav.dataSize = _blockCount * 0x80 * 8 * wav.fmtSamplingSize;
		wav.riffSize = wav.dataSize + 0x24;
		fwrite(&wav, sizeof(wav), 1, fp2);
#endif
	}
	fseek(fp, (int32_t)_dataOffset - 0x800, SEEK_CUR);

	// 音量を設定
	_rva_volume *= volume;

	// デコード
	uint8_t *data2 = new uint8_t[_blockSize];
	if (!data2)
	{
#ifdef HAVE_SNDFILE
		sf_close(fp2);
#else
		fclose(fp2);
#endif
		fclose(fp); return false;
	}
	for (; _blockCount; _blockCount--){
		fread(data2, _blockSize, 1, fp);
		if (!Decode(fp2, data2, _blockSize, _dataOffset)){
			delete[] data2;
#ifdef HAVE_SNDFILE
			sf_close(fp2);
#else
			fclose(fp2);
#endif
			fclose(fp);
			return false;
		}
	}
	delete[] data2;

	// 閉じる
	fclose(fp);
#ifdef HAVE_SNDFILE
	std::string loopheader = "loop:";
	std::string loopstart = "\nloopstart="+std::to_string(_loopStart);
	std::string loopend = "\nloopend="+std::to_string(_loopEnd);
	std::string loopOC = "\nloop0C="+std::to_string(_loop_r0C);
	std::string loopOE = "\nloopOE="+std::to_string(_loop_r0E);
	std::string comment = loopheader+loopstart+loopend+loopOC+loopOE;
	sf_set_string(fp2, SF_STR_COMMENT, comment.c_str());
	sf_close(fp2);
#else
	fclose(fp2);
#endif

	return true;
}

bool clHCA::Encode(const char *filenameWAV, const char *filename, float volume)
{
#ifdef HAVE_SNDFILE
	FILE *out;
	SNDFILE *in;
	SF_INFO si;
	si.format = 0;
	in = sf_open(filenameWAV, SFM_READ, &si);
	if (in == NULL)
		return false;
	if (fopen_s(&out, filename, "wb"))
	{
		sf_close(in);
		return false;
	}
	_samplingRate = si.samplerate;
	_channelCount = si.channels;
	_blockSize = 0x80*8*_samplingRate;
	_rva_volume = volume;
	return true;
#else
	(void)filenameWAV;
	(void)filename;
	(void)volume;
	return false;
#endif
}


bool clHCA::Decode(FILE *fp, void *data, size_t size){

	// ヘッダを解析
	if (!Decode(fp, data, size, 0))return false;
	data = (uint8_t *)data + _dataOffset;
	size -= (int32_t)_dataOffset;

	// デコード
	for (; _blockCount&&size > 0; _blockCount--, data = (uint8_t *)data + _blockSize, size -= (int32_t)_blockSize){
		if (!Decode(fp, data, size, _dataOffset)){
			return false;
		}
	}

	return true;
}
bool clHCA::Decode2(FILE *fp, FILE *fpHCA, size_t size){

	// ヘッダを解析
	uint8_t data[0x200];
	fread(data, sizeof(data), 1, fpHCA);
	if (!Decode(fp, data, sizeof(data), 0))return false;
	fseek(fpHCA, (int32_t)_dataOffset - 0x200, SEEK_CUR);
	size -= (int32_t)_dataOffset;

	// デコード
	uint8_t *data2 = new uint8_t[_blockSize];
	if (!data2)return false;
	for (; _blockCount&&size > 0; _blockCount--, size -= (int32_t)_blockSize){
		fread(data2, _blockSize, 1, fpHCA);
		if (!Decode(fp, data2, _blockSize, _dataOffset)){
			delete[] data2;
			return false;
		}
	}
	delete[] data2;

	return true;
}

//--------------------------------------------------
// デコード
//--------------------------------------------------
bool clHCA::Decode(void *fp, void *data, size_t size, uint32_t address)
{

	// チェック
#ifndef HAVE_SNDFILE
	if (!fp)return false;
#endif
	if (!data)return false;

	// ヘッダ
	if (address == 0)
	{
		uint8_t *s = (uint8_t *)data;

		// HCA
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x00414348){
			if (size < sizeof(stHeader))
				return false;
			stHeader *hca = (stHeader *)s; 
			s += sizeof(stHeader);
			_version = hca->version;
			_revision = hca->revision;
			_dataOffset = bswap(hca->dataOffset);
			//if (!(_version == 1 && _revision == 3))
			//	return false;
			if (size < _dataOffset)
				return false;
			if (CheckSum(hca, _dataOffset) != 0)
				return false;
		}
		else{
			return false;
		}

		// fmt
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x00746D66){
			stFormat *fmt = (stFormat *)s; s += sizeof(stFormat);
			_channelCount = fmt->channelCount;
			_samplingRate = bswap(fmt->samplingRate << 8);
			_blockCount = bswap(fmt->blockCount);
			_fmt_r14 = bswap(fmt->r14);//不明(0xC80)
			_fmt_r16 = bswap(fmt->r16);//不明(0x226)
			if (!(_channelCount >= 1 && _channelCount <= 16))return false;
			if (!(_samplingRate >= 1 && _samplingRate <= 0x7FFFFF))return false;
		}
		else{
			return false;
		}

		// dec
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x00636564)
		{
			stDecode *dec = (stDecode *)s; 
			s += sizeof(stDecode);
			_blockSize = bswap(dec->blockSize);
			_dec_r1E = dec->r1E;
			_dec_r1F = dec->r1F;
			_dec_count1 = dec->count1;
			_dec_count2 = dec->count2;
			_dec_r22 = dec->r22;
			_dec_r23 = dec->r23;
			if (!(/*_dec_r1E >= 0 &&*/ _dec_r1F >= _dec_r1E&&_dec_r1F <= 0x1F))
				return false;
		}
		//comp
		else if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x706D6F63)
		{
			stComp *dec = (stComp *)s;
			s += sizeof(stComp);
			_blockSize = bswap(dec->blockSize); 
			_dec_r1E = dec->v8;
			_dec_r1F = dec->v7;
			//_dec_count1 = dec->v13;
			_dec_count1 = 0x40;
			_dec_count2 = dec->count2;
			_dec_r22 = dec->v9;
			_dec_r23 = dec->v10;
		}
		else
		{
			return false;
		}

		if (!((_blockSize >= 8 && _blockSize <= 0xFFFF) || (_blockSize == 0)))
			return false;

		// vbr
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x00726276){
			stVBR *vbr = (stVBR *)s; s += sizeof(stVBR);
			_vbr_r04 = bswap(vbr->r04);
			_vbr_r06 = bswap(vbr->r06);
			if (!(_blockSize == 0 && _vbr_r04 >= 0 && _vbr_r04 <= 0x1FF))return false;
		}
		else{
			_vbr_r04 = 0;
			_vbr_r06 = 0;
		}

		// ath
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x00687461){
			stATH *ath = (stATH *)s; s+=sizeof(stATH);
			_ath_type = ath->type;
		}
		else{
			_ath_type = 1;
		}

		// loop
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x706F6F6C){
			stLoop *loop = (stLoop *)s; s += sizeof(stLoop);
			_loopStart = bswap(loop->loopStart);
			_loopEnd = bswap(loop->loopEnd);
			_loop_r0C = bswap(loop->r0C);
			_loop_r0E = bswap(loop->r0E);
			if (!(/*_loopStart >= 0 &&*/ _loopStart <= _loopEnd&&_loopEnd < _blockCount))return false;
		}
		else{
			_loopStart = 0;
			_loopEnd = 0;
			_loop_r0C = 0;
			_loop_r0E = 0x400;
		}

		// ciph
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x68706963){
			stCIPH *ciph = (stCIPH *)s; s+=sizeof(stCIPH);
			_ciph_type = bswap(ciph->type);
			if (!(_ciph_type == 0 || _ciph_type == 1 || _ciph_type == 0x38))return false;
		}
		else{
			_ciph_type = 0;
		}

		// rva
		if ((*(uint32_t *)s & 0x7F7F7F7F) == 0x00617672){
			stRVA *rva = (stRVA *)s; s += sizeof(stRVA);
			_rva_volume = bswap(rva->volume);
		}
		else{
			_rva_volume = 1;
		}

		// comm
		//if((*(uint32_t *)s&0x7F7F7F7F)==0x6D6D6F63){
		//	stComment *comm=(stComment *)s;
		//	//
		//}

		// 初期化
		if (!_ath.Init(_ath_type, _samplingRate))return false;
		if (!_ciph.Init(_ciph_type, _ciph_key1, _ciph_key2))return false;
		if (!InitDecode(_channelCount, _dec_r1E, _dec_r1F, _dec_count1, _dec_count2, _dec_r22, _dec_r23))return false;
	}

	// データ
	else if (address >= _dataOffset){
		if (size < _blockSize)return false;
		if (CheckSum(data, _blockSize) != 0)return false;
		_ciph.Mask(data, _blockSize);
		clData d(data, _blockSize);
		Decode(&d);
		int32_t tmp[16*0x80*8];
		int32_t *p = tmp;
		for (int32_t i = 0; i < 8; i++){
			for (int32_t j = 0; j < 0x80; j++){
				for (int32_t k = 0; k < (int32_t)_channelCount; k++){
					fm_t f = _channel[k].wave[i][j] * _rva_volume;
					if (f>1) *p++ = INT32_MAX;
					else if (f < -1) *p++ = INT32_MIN;
					else *p++ = (int32_t)(f * INT32_MAX);
				}
			}
		}
#ifdef HAVE_SNDFILE
		sf_write_int((SNDFILE *)fp, tmp, _channelCount * 0x80 * 8);
#else
		fwrite(&tmp, sizeof(tmp[0]), _channelCount*0x80*8, (FILE *)fp);
#endif
	}

	return true;
}

bool clHCA::Encode(void *fp, void *data, size_t size, uint32_t address)
{
#ifdef HAVE_SNDFILE
	if (address == 0)
	{
		(void)size; ///TODO: backward
		return false;
	}
	else if (address >= _dataOffset){
		fm_t tmp[16*0x80*8];
		fm_t *p = tmp;
#if (UINTPTR_MAX == UINT64_MAX)
		sf_read_double((SNDFILE *)fp, tmp, _channelCount * 0x80 * 8);
#else
		sf_read_float((SNDFILE *)fp, tmp, _channelCount * 0x80 * 8);
#endif
		for (int32_t i = 0; i < 8; i++){
			for (int32_t j = 0; j < 0x80; j++){
				for (int32_t k = 0; k < (int32_t)_channelCount; k++){
					_channel[k].wave[i][j] = *p++ * _rva_volume;
				}
			}
		}
		clData d(data, _blockSize);
		Encode(&d);
/*
		if (size < _blockSize)return false;
		if (CheckSum(data, _blockSize) != 0)return false;
		_ciph.Mask(data, _blockSize);
*/
		return true;
	}
#else
	(void)fp;
	(void)data;
	(void)size;
	(void)address;
#endif

	return false;
}


//--------------------------------------------------
// ATH
//--------------------------------------------------
clHCA::clATH::clATH(){ Init0(); }
bool clHCA::clATH::Init(int32_t type, uint32_t key){
	switch (type){
	case 0:Init0(); break;
	case 1:Init1(key); break;
	default:return false;
	}
	return true;
}

const uint8_t *clHCA::clATH::GetTable(void){
	return _table;
}
void clHCA::clATH::Init0(void){
	memset(_table, 0, sizeof(_table));
}
void clHCA::clATH::Init1(uint32_t key){
	static uint8_t list[] = {
		0x78, 0x5F, 0x56, 0x51, 0x4E, 0x4C, 0x4B, 0x49, 0x48, 0x48, 0x47, 0x46, 0x46, 0x45, 0x45, 0x45,
		0x44, 0x44, 0x44, 0x44, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
		0x42, 0x42, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x40, 0x40, 0x40, 0x40,
		0x40, 0x40, 0x40, 0x40, 0x40, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D,
		0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B,
		0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B,
		0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3B, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C,
		0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3F,
		0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
		0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
		0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
		0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x43, 0x43, 0x43,
		0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x45, 0x45, 0x45, 0x45,
		0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46, 0x46,
		0x46, 0x46, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x48, 0x48, 0x48, 0x48,
		0x48, 0x48, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x4A, 0x4A, 0x4A, 0x4A,
		0x4A, 0x4A, 0x4A, 0x4A, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4C, 0x4C,
		0x4C, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4D, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4E, 0x4F, 0x4F, 0x4F,
		0x4F, 0x4F, 0x4F, 0x50, 0x50, 0x50, 0x50, 0x50, 0x51, 0x51, 0x51, 0x51, 0x51, 0x52, 0x52, 0x52,
		0x52, 0x52, 0x53, 0x53, 0x53, 0x53, 0x54, 0x54, 0x54, 0x54, 0x54, 0x55, 0x55, 0x55, 0x55, 0x56,
		0x56, 0x56, 0x56, 0x57, 0x57, 0x57, 0x57, 0x57, 0x58, 0x58, 0x58, 0x59, 0x59, 0x59, 0x59, 0x5A,
		0x5A, 0x5A, 0x5A, 0x5B, 0x5B, 0x5B, 0x5B, 0x5C, 0x5C, 0x5C, 0x5D, 0x5D, 0x5D, 0x5D, 0x5E, 0x5E,
		0x5E, 0x5F, 0x5F, 0x5F, 0x60, 0x60, 0x60, 0x61, 0x61, 0x61, 0x61, 0x62, 0x62, 0x62, 0x63, 0x63,
		0x63, 0x64, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x66, 0x67, 0x67, 0x67, 0x68, 0x68, 0x68, 0x69,
		0x69, 0x6A, 0x6A, 0x6A, 0x6B, 0x6B, 0x6B, 0x6C, 0x6C, 0x6D, 0x6D, 0x6D, 0x6E, 0x6E, 0x6F, 0x6F,
		0x70, 0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x73, 0x74, 0x74, 0x75, 0x75, 0x76, 0x76,
		0x77, 0x77, 0x78, 0x78, 0x78, 0x79, 0x79, 0x7A, 0x7A, 0x7B, 0x7B, 0x7C, 0x7C, 0x7D, 0x7D, 0x7E,
		0x7E, 0x7F, 0x7F, 0x80, 0x80, 0x81, 0x81, 0x82, 0x83, 0x83, 0x84, 0x84, 0x85, 0x85, 0x86, 0x86,
		0x87, 0x88, 0x88, 0x89, 0x89, 0x8A, 0x8A, 0x8B, 0x8C, 0x8C, 0x8D, 0x8D, 0x8E, 0x8F, 0x8F, 0x90,
		0x90, 0x91, 0x92, 0x92, 0x93, 0x94, 0x94, 0x95, 0x95, 0x96, 0x97, 0x97, 0x98, 0x99, 0x99, 0x9A,
		0x9B, 0x9B, 0x9C, 0x9D, 0x9D, 0x9E, 0x9F, 0xA0, 0xA0, 0xA1, 0xA2, 0xA2, 0xA3, 0xA4, 0xA5, 0xA5,
		0xA6, 0xA7, 0xA7, 0xA8, 0xA9, 0xAA, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAE, 0xAF, 0xB0, 0xB1, 0xB1,
		0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
		0xC0, 0xC1, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD,
		0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD,
		0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xED, 0xEE,
		0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFF, 0xFF,
	};
	for (uint32_t i = 0, v = 0; i < 0x80; i++, v += key){
		uint32_t index = v >> 13;
		if (index >= 0x28E){
			memset(&_table[i], 0xFF, 0x80 - i);
			break;
		}
		_table[i] = list[index];
	}
}

//--------------------------------------------------
// CIPH
//--------------------------------------------------
clHCA::clCIPH::clCIPH(){ Init0(); }
bool clHCA::clCIPH::Init(int32_t type, uint32_t key1, uint32_t key2){
	if (!(key1 | key2))type = 0;
	switch (type){
	case 0:Init0(); break;
	case 1:Init1(); break;
	case 56:Init56(key1, key2); break;
	default:return false;
	}
	return true;
}
void clHCA::clCIPH::Mask(void *data, int32_t size){
	for (uint8_t *d = (uint8_t *)data; size > 0; d++, size--){
		*d = _table[*d];
	}
}
void clHCA::clCIPH::Init0(void){
	for (uint16_t i = 0; i < 0x100; i++)_table[i] = (uint8_t)i;
}
void clHCA::clCIPH::Init1(void){
	for (uint16_t i = 0, v = 0; i < 0xFF; i++){
		v = (v * 13 + 11) & 0xFF;
		if (v == 0 || v == 0xFF)v = (v * 13 + 11) & 0xFF;
		_table[i] = (uint8_t)v;
	}
	_table[0] = 0;
	_table[0xFF] = 0xFF;
}
void clHCA::clCIPH::Init56(uint32_t key1, uint32_t key2) {

	// テーブル1を生成
	uint8_t t1[8];
	if (!key1)key2--;
	key1--;
	for (uint8_t i = 0; i < 7; i++) {
		t1[i] = (uint8_t)key1;
		key1 = (key1 >> 8) | (key2 << 24);
		key2 >>= 8;
	}

	// テーブル2
	uint8_t t2[0x10] = {
		t1[1],
		(uint8_t)(t1[1] ^ t1[6]),
		(uint8_t)(t1[2] ^ t1[3]),
		t1[2],
		(uint8_t)(t1[2] ^ t1[1]),
		(uint8_t)(t1[3] ^ t1[4]),
		t1[3],
		(uint8_t)(t1[3] ^ t1[2]),
		(uint8_t)(t1[4] ^ t1[5]),
		t1[4],
		(uint8_t)(t1[4] ^ t1[3]),
		(uint8_t)(t1[5] ^ t1[6]),
		t1[5],
		(uint8_t)(t1[5] ^ t1[4]),
		(uint8_t)(t1[6] ^ t1[1]),
		t1[6],
	};

	// テーブル3
	uint8_t t3[0x100], t31[0x10], t32[0x10], *t = t3;
	Init56_CreateTable(t31, t1[0]);
	for (int32_t i = 0; i < 0x10; i++) {
		Init56_CreateTable(t32, t2[i]);
		uint8_t v = t31[i] << 4;
		for (int32_t j = 0; j < 0x10; j++) {
			*(t++) = v | t32[j];
		}
	}

	// CIPHテーブル
	t = &_table[1];
	for (int32_t i = 0, v = 0; i < 0x100; i++) {
		v = (v + 0x11) & 0xFF;
		uint8_t a = t3[v];
		if (a != 0 && a != 0xFF)*(t++) = a;
	}
	_table[0] = 0;
	_table[0xFF] = 0xFF;

}
void clHCA::clCIPH::Init56_CreateTable(uint8_t *r, uint8_t key){
	int32_t mul = ((key & 1) << 3) | 5;
	int32_t add = (key & 0xE) | 1;
	key >>= 4;
	for (int32_t i = 0; i < 0x10; i++){
		key = (key*mul + add) & 0xF;
		*(r++) = key;
	}
}

//--------------------------------------------------
// データ
//--------------------------------------------------
clHCA::clData::clData(void *data, int32_t size) :_data((uint8_t *)data), _size(size * 8 - 16), _bit(0){}
int32_t clHCA::clData::CheckBit(int32_t bitSize){
	int32_t v = 0;
	if (_bit + bitSize < _size){
		static int32_t mask[] = {
			0xFFFFFF, 0x7FFFFF, 0x3FFFFF, 0x1FFFFF,
			0x0FFFFF, 0x07FFFF, 0x03FFFF, 0x01FFFF,
		};
		uint8_t *data = &_data[_bit >> 3];
		v = data[0]; v = (v << 8) | data[1]; v = (v << 8) | data[2];
		v &= mask[_bit & 7];
		v >>= 24 - (_bit & 7) - bitSize;
	}
	return v;
}
int32_t clHCA::clData::GetBit(int32_t bitSize){
	int32_t v = CheckBit(bitSize);
	_bit += bitSize;
	return v;
}

void clHCA::clData::AddBit(int32_t bitSize){
	_bit += bitSize;
}

void clHCA::clData::SetBit(int32_t bitSize, int32_t value)
{
	(void)bitSize;
	(void)value; /// TODO: add bits of data
}


//--------------------------------------------------
// デコード
//--------------------------------------------------
bool clHCA::InitDecode(uint32_t channelCount, int32_t a, int32_t b, uint32_t count1, uint32_t count2, uint32_t e, uint32_t f){

	// チェック
	if (!(a == 1 && b == 15))
		return false;

	// 初期化
	_channelCount = channelCount;
	memset(_channel, 0, sizeof(_channel));

	// 種類を設定
	if (f){
		f = e & 0xF;
		e >>= 4;
		if (!e)e = channelCount;
		switch (e){
		case 2:
			for (uint32_t i = 0; i < channelCount; i += e){
				_channel[i + 0].type = 1;
				_channel[i + 1].type = 2;
			}
			break;
		case 3:
			for (uint32_t i = 0; i < channelCount; i += e){
				_channel[i + 0].type = 1;
				_channel[i + 1].type = 2;
			}
			break;
		case 4:
			for (uint32_t i = 0; i < channelCount; i += e){
				_channel[i + 0].type = 1;
				_channel[i + 1].type = 2;
				if (!f){
					_channel[i + 2].type = 1;
					_channel[i + 3].type = 2;
				}
			}
			break;
		case 5:
			for (uint32_t i = 0; i < channelCount; i += e){
				_channel[i + 0].type = 1;
				_channel[i + 1].type = 2;
				if (f <= 2){
					_channel[i + 3].type = 1;
					_channel[i + 4].type = 2;
				}
			}
			break;
		case 6:
		case 7:
			for (uint32_t i = 0; i < channelCount; i += e){
				_channel[i + 0].type = 1;
				_channel[i + 1].type = 2;
				_channel[i + 4].type = 1;
				_channel[i + 5].type = 2;
			}
			break;
		case 8:
			for (uint32_t i = 0; i < channelCount; i += e){
				_channel[i + 0].type = 1;
				_channel[i + 1].type = 2;
				_channel[i + 4].type = 1;
				_channel[i + 5].type = 2;
				_channel[i + 6].type = 1;
				_channel[i + 7].type = 2;
			}
			break;
		}
	}

	// 回数を設定
	for (uint32_t i = 0; i < channelCount; i++){
		_channel[i].count = 1 + ((_channel[i].type != 2) ? count1 : count2);
	}

	return true;
}

void clHCA::Decode(clData *data){
	int32_t magic = data->GetBit(16);//0xFFFF固定
	if (magic == 0xFFFF){
		int32_t a = data->GetBit(9);//不明
		int32_t b = data->GetBit(7);//不明
		for (int32_t i = 0; i < (int32_t)_channelCount; i++){
			_channel[i].Decode1(data);
			_channel[i].Decode2(a, b, _ath.GetTable());
			_channel[i].Decode3();
		}
		for (int32_t i = 0; i < 8; i++){
			for (int32_t j = 0; j < (int32_t)_channelCount; j++){
				_channel[j].Decode4(data);
			}
			for (int32_t j = 0; j < (int32_t)_channelCount; j++){
				_channel[j].Decode5(i, _channel[j+1]);
#if 1
				_channel[j].Encode5(i, _channel[j-1]);
				_channel[j].Decode5(i, _channel[j+1]);
#endif
				_channel[j].Decode6();
				_channel[j].Decode7();
				_channel[j].Decode8(i);
#if 0 // Close
				_channel[j].Encode8(i);
				_channel[j].Decode8(i);
#endif
			}
		}
	}
}

void clHCA::Encode(clData *data)
{
	int32_t a = 0x00, b = 0x00;
	for (int32_t i = 0; i < 8; i++)
	{
		for (int32_t j = _channelCount; j < 0; j--)
		{
			_channel[j].Encode8(i);
			_channel[j].Encode7();
			_channel[j].Encode6();
			_channel[j].Encode5(i, _channel[j-1]);
		}
		for (int32_t j = _channelCount; j < 0; j--)
		{
			_channel[j].Encode4(data);
		}
	}
	for (int32_t i = _channelCount; i < 0; i--)
	{
		_channel[i].Encode3();
		_channel[i].Encode2(&a, &b, _ath.GetTable());
		_channel[i].Encode1(data);
	}

	data->SetBit(16, 0xFFFF);
	data->SetBit(7, b);
	data->SetBit(9, a);
}

//--------------------------------------------------
// デコード第一段階
//   データから値を取得
//--------------------------------------------------
void clHCA::stChannel::Decode1(clData *data){
	int32_t v = data->GetBit(3);
	if (v >= 6){
		for (int32_t i = 0; i < count; i++){
			value[i] = (uint8_t)data->GetBit(6);
		}
	}
	else if (v){
		const int32_t v1 = (1 << v) - 1;
		const int32_t v2 = v1 >> 1;
		int32_t x = data->GetBit(6);
		value[0] = (uint8_t)x;
		for (int32_t i = 1; i < count; i++){
			const int32_t y = data->GetBit(v);
			if (y != v1){
				x += y - v2;
			}
			else{
				x = data->GetBit(6);
			}
			value[i] = (uint8_t)x;
		}
	}
	else{
		memset(value, 0, 0x80);
	}
	if (type == 2){
		v = data->CheckBit(4);
		if (v < 15){
			for (int32_t i = 0; i < 8; i++){
				value2[i] = (uint8_t)data->GetBit(4);
			}
		}
	}
}


void clHCA::stChannel::Encode1(clData *data)
{
	(void)data; /// TODO: work backward
}

//--------------------------------------------------
// デコード第二段階
//   値からスケールを計算
//--------------------------------------------------
void clHCA::stChannel::Decode2(int32_t a, int32_t b, const uint8_t *ath){
	static const uint8_t index[] = {
		0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D,
		0x0D, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0C,
		0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
		0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x09,
		0x09, 0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08,
		0x08, 0x08, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04,
		0x04, 0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02,
		0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	};
	const int32_t c = (a << 8) - b;
	for (int32_t i = 0; i < count; i++){
		int32_t v = value[i];
		if (v){
			v = ath[i] + ((c + i) >> 8) - ((v * 5) >> 1) + 1;
			if (v < 0)v = 15;
			else if (v >= 0x39)v = 1;
			else v = index[v];
		}
		scale[i] = (uint8_t)v;
	}
	memset(&scale[count], 0, 0x80 - count);
}

void clHCA::stChannel::Encode2(int32_t *a, int32_t *b, const uint8_t *ath)
{
	(void)a; /// TODO: backward
	(void)b;
	(void)ath;
}


static const fm_t valueFloat[0x40] = {  /// TODO: F/D
0.000000158838332708910456858575344085693359375,
0.0000002116413639896563836373388767242431640625,
0.000000281997841966585838235914707183837890625,
0.00000037574312727883807383477687835693359375,
0.00000050065233381246798671782016754150390625,
0.000000667085487293661572039127349853515625,
0.0000008888464435585774481296539306640625,
0.000001184327857117750681936740875244140625,
0.0000015780370858919923193752765655517578125,
0.000002102627831845893524587154388427734375,
0.00000280160975307808257639408111572265625,
0.000003732956201929482631385326385498046875,
0.00000497391238241107203066349029541015625,
0.00000662740285406471230089664459228515625,
0.0000088305669123656116425991058349609375,
0.000011766134775825776159763336181640625,
0.000015677580449846573173999786376953125,
0.000020889319785055704414844512939453125,
0.000027833608328364789485931396484375,
0.0000370864072465337812900543212890625,
0.00004941513543599285185337066650390625,
0.0000658423305139876902103424072265625,
0.000087730470113456249237060546875,
0.0001168949311249889433383941650390625,
0.000155754605657421052455902099609375,
0.00020753251737914979457855224609375,
0.00027652308926917612552642822265625,
0.00036844835267402231693267822265625,
0.0004909325507469475269317626953125,
0.000654134550131857395172119140625,
0.0008715901640243828296661376953125,
0.00116133503615856170654296875,
0.00154740060679614543914794921875,
0.0020618070848286151885986328125,
0.00274721882306039333343505859375,
0.0036604837514460086822509765625,
0.0048773474991321563720703125,
0.0064987367950379848480224609375,
0.0086591280996799468994140625,
0.0115377046167850494384765625,
0.015373212285339832305908203125,
0.020483769476413726806640625,
0.02729324065148830413818359375,
0.036366403102874755859375,
0.0484557785093784332275390625,
0.064564056694507598876953125,
0.08602724969387054443359375,
0.114625506103038787841796875,
0.15273074805736541748046875,
0.203503429889678955078125,
0.271154582500457763671875,
0.3612951934337615966796875,
0.4814014732837677001953125,
0.64143502712249755859375,
0.8546688556671142578125,
1.13878858089447021484375,
1.5173590183258056640625,
2.021778583526611328125,
2.6938836574554443359375,
3.5894181728363037109375,
4.782657623291015625,
6.372568607330322265625,
8.49101734161376953125,
11.31370830535888671875,
};

static const fm_t scaleFloat[0x10] = {
0.0/   1.0,
2.0/   3.0,
2.0/   5.0,
2.0/   7.0,
2.0/   9.0,
2.0/  11.0,
2.0/  13.0,
2.0/  15.0,
2.0/  31.0,
2.0/  63.0,
2.0/ 127.0,
2.0/ 255.0,
2.0/ 511.0,
2.0/1023.0,
2.0/2047.0,
2.0/4095.0,
};

//--------------------------------------------------
// デコード第三段階
//   値とスケールからブロック内基準値を計算
//--------------------------------------------------
void clHCA::stChannel::Decode3(void){
	for (int32_t i = 0; i < count; i++){
		base[i] = valueFloat[value[i]] * scaleFloat[scale[i]];
	}
}

void clHCA::stChannel::Encode3(void){
}

//--------------------------------------------------
// デコード第四段階
//   データからブロック内ブロック値を取得、計算
//--------------------------------------------------
void clHCA::stChannel::Decode4(clData *data){
	static const uint8_t list1[] = {
		0, 2, 3, 3, 4, 4, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12,
	};
	static const uint8_t list2[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 2, 2, 2, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	};
	static const fm_t list3[] = {
		+0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0,
		+0, +0, +1, -1, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0, +0,
		+0, +0, +1, +1, -1, -1, +2, -2, +0, +0, +0, +0, +0, +0, +0, +0,
		+0, +0, +1, -1, +2, -2, +3, -3, +0, +0, +0, +0, +0, +0, +0, +0,
		+0, +0, +1, +1, -1, -1, +2, +2, -2, -2, +3, +3, -3, -3, +4, -4,
		+0, +0, +1, +1, -1, -1, +2, +2, -2, -2, +3, -3, +4, -4, +5, -5,
		+0, +0, +1, +1, -1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6,
		+0, +0, +1, -1, +2, -2, +3, -3, +4, -4, +5, -5, +6, -6, +7, -7,
	};
	for (int32_t i = 0; i < count; i++){
		fm_t f;
		const int32_t s = scale[i];
		const int32_t bitSize = list1[s];
		int32_t v = data->GetBit(bitSize);
		if (s >= 8){
			v = (1 - ((v & 1) << 1))*(v >> 1);
			if (!v)data->AddBit(-1);
			f = (fm_t)v;
		}
		else{
			v += s << 4;
			data->AddBit(list2[v] - bitSize);
			f = list3[v];
		}
		block[i] = base[i] * f;
	}
	memset(&block[count], 0, sizeof(block[count])*(0x80 - count));
}

void clHCA::stChannel::Encode4(clData *data){
	(void)data;  /// TODO: backward
}

//--------------------------------------------------
// デコード第五段階
//--------------------------------------------------
void clHCA::stChannel::Decode5(int32_t index, clHCA::stChannel s)
{
	if (type != 1)
		return;
	int32_t i = s.count;
	const uint8_t x = s.value2[index];
	fm_t *p = &block[i];
	fm_t *n = &s.block[i];
	if (x > 13)
	{
		for (; i < count; i++)
		{
			*n++ = 0.0;
			*p++ = 0.0;
		}
	}
	else
	{
		const fm_t f1 = ((-0.0+x)/7.0);
		const fm_t f2 = ((14.0-x)/7.0);
		for (; i < count; i++)
		{
			const fm_t t = *p;
			*(n++) = t*f1;
			*(p++) = t*f2;
		}
	}
	assert(i == s.count+count);
	assert(p == block);
	assert(n == s.block);
}

void clHCA::stChannel::Encode5(int32_t index, clHCA::stChannel s)
{
	return; (void)index; (void)s;
}

//--------------------------------------------------
// デコード第六段階
//--------------------------------------------------
void clHCA::stChannel::Decode6(void){
	fm_t *s = block;
	fm_t *d = wav1;
	int32_t i, count1, count2;
	for (i = 0, count1 = 1, count2 = 0x40; i < 7; i++, count1 <<= 1, count2 >>= 1){
		fm_t *d1 = d;
		fm_t *d2 = &d[count2];
		for (int32_t j = 0; j < count1; j++){
			for (int32_t k = 0; k < count2; k++){
				const fm_t a = *(s++);
				const fm_t b = *(s++);
				*(d1++) = b + a;
				*(d2++) = a - b;
			}
			d1 += count2;
			d2 += count2;
		}
		fm_t *w = &s[-0x80]; s = d; d = w;
	}
}

void clHCA::stChannel::Encode6(void)
{
	/// TOOD: backward
}

static const fm_t Float1List[7][0x40] = { /// TODO: F/D
	{
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625,
0.081660188734531402587890625
	},
	{
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000,
0.980785250663757324218750000,
0.831469595432281494140625000
	},
	{
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
0.995184719562530517578125000,
0.956940352916717529296875000,
0.881921291351318359375000000,
0.773010432720184326171875000,
	},
	{
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
0.998795449733734130859375000,
0.989176511764526367187500000,
0.970031261444091796875000000,
0.941544055938720703125000000,
0.903989315032958984375000000,
0.857728600502014160156250000,
0.803207516670227050781250000,
0.740951120853424072265625000,
	},
	{
0.999698817729949951171875000,
0.997290432453155517578125000,
0.992479562759399414062500000,
0.985277652740478515625000000,
0.975702106952667236328125000,
0.963776051998138427734375000,
0.949528157711029052734375000,
0.932992815971374511718750000,
0.914209783077239990234375000,
0.893224298954010009765625000,
0.870086967945098876953125000,
0.844853579998016357421875000,
0.817584812641143798828125000,
0.788346409797668457031250000,
0.757208824157714843750000000,
0.724247097969055175781250000,
0.999698817729949951171875000,
0.997290432453155517578125000,
0.992479562759399414062500000,
0.985277652740478515625000000,
0.975702106952667236328125000,
0.963776051998138427734375000,
0.949528157711029052734375000,
0.932992815971374511718750000,
0.914209783077239990234375000,
0.893224298954010009765625000,
0.870086967945098876953125000,
0.844853579998016357421875000,
0.817584812641143798828125000,
0.788346409797668457031250000,
0.757208824157714843750000000,
0.724247097969055175781250000,
0.999698817729949951171875000,
0.997290432453155517578125000,
0.992479562759399414062500000,
0.985277652740478515625000000,
0.975702106952667236328125000,
0.963776051998138427734375000,
0.949528157711029052734375000,
0.932992815971374511718750000,
0.914209783077239990234375000,
0.893224298954010009765625000,
0.870086967945098876953125000,
0.844853579998016357421875000,
0.817584812641143798828125000,
0.788346409797668457031250000,
0.757208824157714843750000000,
0.724247097969055175781250000,
0.999698817729949951171875000,
0.997290432453155517578125000,
0.992479562759399414062500000,
0.985277652740478515625000000,
0.975702106952667236328125000,
0.963776051998138427734375000,
0.949528157711029052734375000,
0.932992815971374511718750000,
0.914209783077239990234375000,
0.893224298954010009765625000,
0.870086967945098876953125000,
0.844853579998016357421875000,
0.817584812641143798828125000,
0.788346409797668457031250000,
0.757208824157714843750000000,
0.724247097969055175781250000,
	},
	{
0.999924719333648681640625000,
0.999322354793548583984375000,
0.998118102550506591796875000,
0.996312618255615234375000000,
0.993906974792480468750000000,
0.990902662277221679687500000,
0.987301409244537353515625000,
0.983105480670928955078125000,
0.978317379951477050781250000,
0.972939968109130859375000000,
0.966976463794708251953125000,
0.960430502891540527343750000,
0.953306019306182861328125000,
0.945607304573059082031250000,
0.937339007854461669921875000,
0.928506076335906982421875000,
0.919113874435424804687500000,
0.909168004989624023437500000,
0.898674488067626953125000000,
0.887639641761779785156250000,
0.876070082187652587890625000,
0.863972842693328857421875000,
0.851355195045471191406250000,
0.838224709033966064453125000,
0.824589312076568603515625000,
0.810457170009613037109375000,
0.795836925506591796875000000,
0.780737221240997314453125000,
0.765167236328125000000000000,
0.749136388301849365234375000,
0.732654273509979248046875000,
0.715730845928192138671875000,
0.999924719333648681640625000,
0.999322354793548583984375000,
0.998118102550506591796875000,
0.996312618255615234375000000,
0.993906974792480468750000000,
0.990902662277221679687500000,
0.987301409244537353515625000,
0.983105480670928955078125000,
0.978317379951477050781250000,
0.972939968109130859375000000,
0.966976463794708251953125000,
0.960430502891540527343750000,
0.953306019306182861328125000,
0.945607304573059082031250000,
0.937339007854461669921875000,
0.928506076335906982421875000,
0.919113874435424804687500000,
0.909168004989624023437500000,
0.898674488067626953125000000,
0.887639641761779785156250000,
0.876070082187652587890625000,
0.863972842693328857421875000,
0.851355195045471191406250000,
0.838224709033966064453125000,
0.824589312076568603515625000,
0.810457170009613037109375000,
0.795836925506591796875000000,
0.780737221240997314453125000,
0.765167236328125000000000000,
0.749136388301849365234375000,
0.732654273509979248046875000,
0.715730845928192138671875000,
	},
	{
0.999981164932250976562500000,
0.999830603599548339843750000,
0.999529421329498291015625000,
0.999077737331390380859375000,
0.998475551605224609375000000,
0.997723042964935302734375000,
0.996820271015167236328125000,
0.995767414569854736328125000,
0.994564592838287353515625000,
0.993211925029754638671875000,
0.991709768772125244140625000,
0.990058183670043945312500000,
0.988257586956024169921875000,
0.986308097839355468750000000,
0.984210073947906494140625000,
0.981963872909545898437500000,
0.979569792747497558593750000,
0.977028131484985351562500000,
0.974339365959167480468750000,
0.971503913402557373046875000,
0.968522071838378906250000000,
0.965394437313079833984375000,
0.962121427059173583984375000,
0.958703458309173583984375000,
0.955141186714172363281250000,
0.951435029506683349609375000,
0.947585582733154296875000000,
0.943593442440032958984375000,
0.939459204673767089843750000,
0.935183525085449218750000000,
0.930766940116882324218750000,
0.926210224628448486328125000,
0.921514034271240234375000000,
0.916679084300994873046875000,
0.911706030368804931640625000,
0.906595706939697265625000000,
0.901348829269409179687500000,
0.895966231822967529296875000,
0.890448749065399169921875000,
0.884797096252441406250000000,
0.879012227058410644531250000,
0.873094975948333740234375000,
0.867046236991882324218750000,
0.860866963863372802734375000,
0.854557991027832031250000000,
0.848120331764221191406250000,
0.841554999351501464843750000,
0.834862887859344482421875000,
0.828045070171356201171875000,
0.821102499961853027343750000,
0.814036309719085693359375000,
0.806847572326660156250000000,
0.799537241458892822265625000,
0.792106568813323974609375000,
0.784556567668914794921875000,
0.776888489723205566406250000,
0.769103348255157470703125000,
0.761202394962310791015625000,
0.753186821937561035156250000,
0.745057761669158935546875000,
0.736816585063934326171875000,
0.728464365005493164062500000,
0.720002532005310058593750000,
0.711432218551635742187500000,
	}
};

static const fm_t Float2List[7][0x40] = {
	{
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
0.0338247567415237426757812500000000000000000000,
-0.0338247567415237426757812500000000000000000000,
	},
	{
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
-0.1950903236865997314453125000000000000000000000,
-0.5555702447891235351562500000000000000000000000,
0.1950903236865997314453125000000000000000000000,
0.5555702447891235351562500000000000000000000000,
	},
	{
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
0.0980171412229537963867187500000000000000000000,
0.2902846634387969970703125000000000000000000000,
0.4713967442512512207031250000000000000000000000,
0.6343932747840881347656250000000000000000000000,
-0.0980171412229537963867187500000000000000000000,
-0.2902846634387969970703125000000000000000000000,
-0.4713967442512512207031250000000000000000000000,
-0.6343932747840881347656250000000000000000000000,
	},
	{
-0.0490676760673522949218750000000000000000000000,
-0.1467304676771163940429687500000000000000000000,
-0.2429801821708679199218750000000000000000000000,
-0.3368898630142211914062500000000000000000000000,
-0.4275550842285156250000000000000000000000000000,
-0.5141027569770812988281250000000000000000000000,
-0.5956993103027343750000000000000000000000000000,
-0.6715589761734008789062500000000000000000000000,
0.0490676760673522949218750000000000000000000000,
0.1467304676771163940429687500000000000000000000,
0.2429801821708679199218750000000000000000000000,
0.3368898630142211914062500000000000000000000000,
0.4275550842285156250000000000000000000000000000,
0.5141027569770812988281250000000000000000000000,
0.5956993103027343750000000000000000000000000000,
0.6715589761734008789062500000000000000000000000,
0.0490676760673522949218750000000000000000000000,
0.1467304676771163940429687500000000000000000000,
0.2429801821708679199218750000000000000000000000,
0.3368898630142211914062500000000000000000000000,
0.4275550842285156250000000000000000000000000000,
0.5141027569770812988281250000000000000000000000,
0.5956993103027343750000000000000000000000000000,
0.6715589761734008789062500000000000000000000000,
-0.0490676760673522949218750000000000000000000000,
-0.1467304676771163940429687500000000000000000000,
-0.2429801821708679199218750000000000000000000000,
-0.3368898630142211914062500000000000000000000000,
-0.4275550842285156250000000000000000000000000000,
-0.5141027569770812988281250000000000000000000000,
-0.5956993103027343750000000000000000000000000000,
-0.6715589761734008789062500000000000000000000000,
0.0490676760673522949218750000000000000000000000,
0.1467304676771163940429687500000000000000000000,
0.2429801821708679199218750000000000000000000000,
0.3368898630142211914062500000000000000000000000,
0.4275550842285156250000000000000000000000000000,
0.5141027569770812988281250000000000000000000000,
0.5956993103027343750000000000000000000000000000,
0.6715589761734008789062500000000000000000000000,
-0.0490676760673522949218750000000000000000000000,
-0.1467304676771163940429687500000000000000000000,
-0.2429801821708679199218750000000000000000000000,
-0.3368898630142211914062500000000000000000000000,
-0.4275550842285156250000000000000000000000000000,
-0.5141027569770812988281250000000000000000000000,
-0.5956993103027343750000000000000000000000000000,
-0.6715589761734008789062500000000000000000000000,
-0.0490676760673522949218750000000000000000000000,
-0.1467304676771163940429687500000000000000000000,
-0.2429801821708679199218750000000000000000000000,
-0.3368898630142211914062500000000000000000000000,
-0.4275550842285156250000000000000000000000000000,
-0.5141027569770812988281250000000000000000000000,
-0.5956993103027343750000000000000000000000000000,
-0.6715589761734008789062500000000000000000000000,
0.0490676760673522949218750000000000000000000000,
0.1467304676771163940429687500000000000000000000,
0.2429801821708679199218750000000000000000000000,
0.3368898630142211914062500000000000000000000000,
0.4275550842285156250000000000000000000000000000,
0.5141027569770812988281250000000000000000000000,
0.5956993103027343750000000000000000000000000000,
0.6715589761734008789062500000000000000000000000,
	},
	{
-0.0245412290096282958984375000000000000000000000,
-0.0735645666718482971191406250000000000000000000,
-0.1224106773734092712402343750000000000000000000,
-0.1709618866443634033203125000000000000000000000,
-0.2191012352705001831054687500000000000000000000,
-0.2667127549648284912109375000000000000000000000,
-0.3136817514896392822265625000000000000000000000,
-0.3598950505256652832031250000000000000000000000,
-0.4052413105964660644531250000000000000000000000,
-0.4496113359928131103515625000000000000000000000,
-0.4928981959819793701171875000000000000000000000,
-0.5349976420402526855468750000000000000000000000,
-0.5758081674575805664062500000000000000000000000,
-0.6152315735816955566406250000000000000000000000,
-0.6531728506088256835937500000000000000000000000,
-0.6895405650138854980468750000000000000000000000,
0.0245412290096282958984375000000000000000000000,
0.0735645666718482971191406250000000000000000000,
0.1224106773734092712402343750000000000000000000,
0.1709618866443634033203125000000000000000000000,
0.2191012352705001831054687500000000000000000000,
0.2667127549648284912109375000000000000000000000,
0.3136817514896392822265625000000000000000000000,
0.3598950505256652832031250000000000000000000000,
0.4052413105964660644531250000000000000000000000,
0.4496113359928131103515625000000000000000000000,
0.4928981959819793701171875000000000000000000000,
0.5349976420402526855468750000000000000000000000,
0.5758081674575805664062500000000000000000000000,
0.6152315735816955566406250000000000000000000000,
0.6531728506088256835937500000000000000000000000,
0.6895405650138854980468750000000000000000000000,
0.0245412290096282958984375000000000000000000000,
0.0735645666718482971191406250000000000000000000,
0.1224106773734092712402343750000000000000000000,
0.1709618866443634033203125000000000000000000000,
0.2191012352705001831054687500000000000000000000,
0.2667127549648284912109375000000000000000000000,
0.3136817514896392822265625000000000000000000000,
0.3598950505256652832031250000000000000000000000,
0.4052413105964660644531250000000000000000000000,
0.4496113359928131103515625000000000000000000000,
0.4928981959819793701171875000000000000000000000,
0.5349976420402526855468750000000000000000000000,
0.5758081674575805664062500000000000000000000000,
0.6152315735816955566406250000000000000000000000,
0.6531728506088256835937500000000000000000000000,
0.6895405650138854980468750000000000000000000000,
-0.0245412290096282958984375000000000000000000000,
-0.0735645666718482971191406250000000000000000000,
-0.1224106773734092712402343750000000000000000000,
-0.1709618866443634033203125000000000000000000000,
-0.2191012352705001831054687500000000000000000000,
-0.2667127549648284912109375000000000000000000000,
-0.3136817514896392822265625000000000000000000000,
-0.3598950505256652832031250000000000000000000000,
-0.4052413105964660644531250000000000000000000000,
-0.4496113359928131103515625000000000000000000000,
-0.4928981959819793701171875000000000000000000000,
-0.5349976420402526855468750000000000000000000000,
-0.5758081674575805664062500000000000000000000000,
-0.6152315735816955566406250000000000000000000000,
-0.6531728506088256835937500000000000000000000000,
-0.6895405650138854980468750000000000000000000000,
	},
	{
-0.0122715383768081665039062500000000000000000000,
-0.0368072241544723510742187500000000000000000000,
-0.0613207370042800903320312500000000000000000000,
-0.0857973098754882812500000000000000000000000000,
-0.1102222055196762084960937500000000000000000000,
-0.1345807015895843505859375000000000000000000000,
-0.1588581502437591552734375000000000000000000000,
-0.1830398887395858764648437500000000000000000000,
-0.2071113735437393188476562500000000000000000000,
-0.2310581058263778686523437500000000000000000000,
-0.2548656463623046875000000000000000000000000000,
-0.2785196900367736816406250000000000000000000000,
-0.3020059466361999511718750000000000000000000000,
-0.3253102898597717285156250000000000000000000000,
-0.3484186828136444091796875000000000000000000000,
-0.3713172078132629394531250000000000000000000000,
-0.3939920365810394287109375000000000000000000000,
-0.4164295494556427001953125000000000000000000000,
-0.4386162459850311279296875000000000000000000000,
-0.4605387151241302490234375000000000000000000000,
-0.4821837842464447021484375000000000000000000000,
-0.5035383701324462890625000000000000000000000000,
-0.5245896577835083007812500000000000000000000000,
-0.5453249812126159667968750000000000000000000000,
-0.5657318234443664550781250000000000000000000000,
-0.5857978463172912597656250000000000000000000000,
-0.6055110692977905273437500000000000000000000000,
-0.6248595118522644042968750000000000000000000000,
-0.6438315510749816894531250000000000000000000000,
-0.6624158024787902832031250000000000000000000000,
-0.6806010007858276367187500000000000000000000000,
-0.6983762383460998535156250000000000000000000000,
0.0122715383768081665039062500000000000000000000,
0.0368072241544723510742187500000000000000000000,
0.0613207370042800903320312500000000000000000000,
0.0857973098754882812500000000000000000000000000,
0.1102222055196762084960937500000000000000000000,
0.1345807015895843505859375000000000000000000000,
0.1588581502437591552734375000000000000000000000,
0.1830398887395858764648437500000000000000000000,
0.2071113735437393188476562500000000000000000000,
0.2310581058263778686523437500000000000000000000,
0.2548656463623046875000000000000000000000000000,
0.2785196900367736816406250000000000000000000000,
0.3020059466361999511718750000000000000000000000,
0.3253102898597717285156250000000000000000000000,
0.3484186828136444091796875000000000000000000000,
0.3713172078132629394531250000000000000000000000,
0.3939920365810394287109375000000000000000000000,
0.4164295494556427001953125000000000000000000000,
0.4386162459850311279296875000000000000000000000,
0.4605387151241302490234375000000000000000000000,
0.4821837842464447021484375000000000000000000000,
0.5035383701324462890625000000000000000000000000,
0.5245896577835083007812500000000000000000000000,
0.5453249812126159667968750000000000000000000000,
0.5657318234443664550781250000000000000000000000,
0.5857978463172912597656250000000000000000000000,
0.6055110692977905273437500000000000000000000000,
0.6248595118522644042968750000000000000000000000,
0.6438315510749816894531250000000000000000000000,
0.6624158024787902832031250000000000000000000000,
0.6806010007858276367187500000000000000000000000,
0.6983762383460998535156250000000000000000000000,
	},
	{
-0.0061358846724033355712890625000000000000000000,
-0.0184067301452159881591796875000000000000000000,
-0.0306748040020465850830078125000000000000000000,
-0.0429382584989070892333984375000000000000000000,
-0.0551952458918094635009765625000000000000000000,
-0.0674439221620559692382812500000000000000000000,
-0.0796824395656585693359375000000000000000000000,
-0.0919089540839195251464843750000000000000000000,
-0.1041216328740119934082031250000000000000000000,
-0.1163186281919479370117187500000000000000000000,
-0.1284981071949005126953125000000000000000000000,
-0.1406582444906234741210937500000000000000000000,
-0.1527971923351287841796875000000000000000000000,
-0.1649131178855895996093750000000000000000000000,
-0.1770042181015014648437500000000000000000000000,
-0.1890686601400375366210937500000000000000000000,
-0.2011046409606933593750000000000000000000000000,
-0.2131103128194808959960937500000000000000000000,
-0.2250839173793792724609375000000000000000000000,
-0.2370236068964004516601562500000000000000000000,
-0.2489276081323623657226562500000000000000000000,
-0.2607941031455993652343750000000000000000000000,
-0.2726213634014129638671875000000000000000000000,
-0.2844075262546539306640625000000000000000000000,
-0.2961508929729461669921875000000000000000000000,
-0.3078496456146240234375000000000000000000000000,
-0.3195020258426666259765625000000000000000000000,
-0.3311063051223754882812500000000000000000000000,
-0.3426607251167297363281250000000000000000000000,
-0.3541635274887084960937500000000000000000000000,
-0.3656129837036132812500000000000000000000000000,
-0.3770074248313903808593750000000000000000000000,
-0.3883450329303741455078125000000000000000000000,
-0.3996241986751556396484375000000000000000000000,
-0.4108431637287139892578125000000000000000000000,
-0.4220002591609954833984375000000000000000000000,
-0.4330938160419464111328125000000000000000000000,
-0.4441221356391906738281250000000000000000000000,
-0.4550835788249969482421875000000000000000000000,
-0.4659765064716339111328125000000000000000000000,
-0.4767992198467254638671875000000000000000000000,
-0.4875501692295074462890625000000000000000000000,
-0.4982276558876037597656250000000000000000000000,
-0.5088301301002502441406250000000000000000000000,
-0.5193560123443603515625000000000000000000000000,
-0.5298036336898803710937500000000000000000000000,
-0.5401714444160461425781250000000000000000000000,
-0.5504579544067382812500000000000000000000000000,
-0.5606615543365478515625000000000000000000000000,
-0.5707807540893554687500000000000000000000000000,
-0.5808139443397521972656250000000000000000000000,
-0.5907596945762634277343750000000000000000000000,
-0.6006164550781250000000000000000000000000000000,
-0.6103827953338623046875000000000000000000000000,
-0.6200572252273559570312500000000000000000000000,
-0.6296382546424865722656250000000000000000000000,
-0.6391244530677795410156250000000000000000000000,
-0.6485143899917602539062500000000000000000000000,
-0.6578066945075988769531250000000000000000000000,
-0.6669999361038208007812500000000000000000000000,
-0.6760926842689514160156250000000000000000000000,
-0.6850836873054504394531250000000000000000000000,
-0.6939714550971984863281250000000000000000000000,
-0.7027547359466552734375000000000000000000000000,
	}
};

//--------------------------------------------------
// デコード第七段階
//--------------------------------------------------
void clHCA::stChannel::Decode7(void){
	fm_t wav[0x80];
	fm_t *s = wav1;
	fm_t *d = wav;
	int32_t i, count1, count2, j, k;
	for (i = 0, count1 = 0x40, count2 = 1; i < 7; i++, count1 >>= 1, count2 <<= 1){
		const fm_t *list1Float = Float1List[i];
		const fm_t *list2Float = Float2List[i];
		const fm_t *s1 = s;
		const fm_t *s2 = &s1[count2];
		fm_t *d1 = d;
		fm_t *d2 = &d1[count2 * 2 - 1];
		for (j = 0; j < count1; j++){
			for (k = 0; k < count2; k++){
				const fm_t a = *(s1++);
				const fm_t b = *(s2++);
				const fm_t c = *(list1Float++);
				const fm_t fd = *(list2Float++);
				*(d1++) = a*c - b*fd;
				*(d2--) = a*fd + b*c;
			}
			s1 += count2;
			s2 += count2;
			d1 += count2;
			d2 += count2 * 3;
		}
		fm_t *w = s; s = d; d = w;
	}
	assert(count2 == 0x80);
	assert(count1 = 0x10);
	assert(i == 7);
	assert(d = &wav[0x80]);
	d = wav2;
	for (i = 0; i < 0x80; i++){
		*(d++) = *(s++);
	}
	assert(s == &wav[0x80]);
	assert(d == &wav2[0x80]);
}

void clHCA::stChannel::Encode7(void)
{
	fm_t wav[0x80];
	fm_t *d = &wav2[0x80];
	fm_t *s = &wav[0x80];
	int32_t i, count1, count2;
	for (i = 0x80; i > 0; i--){
		*(d--) = *(s--);
	}
	d = &wav[0x80];
	for (i = 7, count1 = 0x10, count2 = 0x80; i > 0; i--, count1 <<= 1, count2 >>= 1){
		/// TOOD: backward
	}
}



static const fm_t floatList8[2][0x40] = { /// TODO: F/D
{
0.000690533779561519622802734375,
0.00197623483836650848388671875,
0.00367386452853679656982421875,
0.0057242400944232940673828125,
0.008096703328192234039306640625,
0.010773181915283203125,
0.0137425176799297332763671875,
0.01699785701930522918701171875,
0.0205352641642093658447265625,
0.02435290254652500152587890625,
0.0284505188465118408203125,
0.0328290946781635284423828125,
0.03749062120914459228515625,
0.04243789613246917724609375,
0.0476744286715984344482421875,
0.0532043017446994781494140625,
0.0590321123600006103515625,
0.06516288220882415771484375,
0.071602009236812591552734375,
0.0783552229404449462890625,
0.08542849123477935791015625,
0.09282802045345306396484375,
0.100560151040554046630859375,
0.108631350100040435791015625,
0.117048121988773345947265625,
0.12581698596477508544921875,
0.1349443495273590087890625,
0.1444365084171295166015625,
0.15429951250553131103515625,
0.1645391285419464111328125,
0.17516072094440460205078125,
0.18616916239261627197265625,
0.19756872951984405517578125,
0.20936296880245208740234375,
0.22155462205410003662109375,
0.23414541780948638916015625,
0.24713599681854248046875,
0.260525763034820556640625,
0.2743127048015594482421875,
0.2884931862354278564453125,
0.3030619323253631591796875,
0.3180117309093475341796875,
0.3333333432674407958984375,
0.349015295505523681640625,
0.365043818950653076171875,
0.3814027011394500732421875,
0.3980731070041656494140625,
0.415033519268035888671875,
0.4322597980499267578125,
0.44972503185272216796875,
0.4673995673656463623046875,
0.4852511584758758544921875,
0.503244936466217041015625,
0.52134382724761962890625,
0.539508521556854248046875,
0.557697772979736328125,
0.575868904590606689453125,
0.59397804737091064453125,
0.61198055744171142578125,
0.62983143329620361328125,
0.647486031055450439453125,
0.664900243282318115234375,
0.682031154632568359375,
0.698837578296661376953125,
},
{
-0.71528041362762451171875,
-0.73132312297821044921875,
-0.74693214893341064453125,
-0.762077331542968750,
-0.77673184871673583984375,
-0.7908728122711181640625,
-0.8044812679290771484375,
-0.817542016506195068359375,
-0.830044090747833251953125,
-0.841980159282684326171875,
-0.85334670543670654296875,
-0.864143788814544677734375,
-0.874374806880950927734375,
-0.88404619693756103515625,
-0.893167078495025634765625,
-0.901749134063720703125,
-0.90980613231658935546875,
-0.917353689670562744140625,
-0.924408972263336181640625,
-0.93099033832550048828125,
-0.937117040157318115234375,
-0.942809045314788818359375,
-0.948086798191070556640625,
-0.95297086238861083984375,
-0.957481920719146728515625,
-0.961640536785125732421875,
-0.965466916561126708984375,
-0.9689807891845703125,
-0.9722015857696533203125,
-0.9751479625701904296875,
-0.977837979793548583984375,
-0.980289041996002197265625,
-0.982517719268798828125,
-0.98453986644744873046875,
-0.986370563507080078125,
-0.98802411556243896484375,
-0.989514052867889404296875,
-0.99085319042205810546875,
-0.992053449153900146484375,
-0.99312627315521240234375,
-0.99408209323883056640625,
-0.9949309825897216796875,
-0.995682179927825927734375,
-0.9963443279266357421875,
-0.996925532817840576171875,
-0.99743330478668212890625,
-0.99787461757659912109375,
-0.99825608730316162109375,
-0.99858367443084716796875,
-0.998862922191619873046875,
-0.99909913539886474609375,
-0.999296963214874267578125,
-0.999460995197296142578125,
-0.999595224857330322265625,
-0.99970340728759765625,
-0.99978911876678466796875,
-0.999855518341064453125,
-0.99990558624267578125,
-0.99994194507598876953125,
-0.99996721744537353515625,
-0.999983608722686767578125,
-0.999993264675140380859375,
-0.999998033046722412109375,
-0.9999997615814208984375,
}
};

//--------------------------------------------------
// デコード第八段階
//--------------------------------------------------
void clHCA::stChannel::Decode8(int32_t index){
	const fm_t *s1 = &wav2[0x40];
	fm_t *s2 = wav3;
	fm_t *d = &wave[index][0];
	const fm_t *listFloatp = floatList8[0];
	int32_t i;
	for (i = 0; i < 0x40; i++){
		*(d++) = *(s1++)**(listFloatp++) + *(s2++);
	}
	for (i = 0; i < 0x40; i++){
		*(d++) = *(listFloatp++)**(--s1) - *(s2++);
	}
	s1--;
	assert(d == &wave[index][0x80]);
	d = wav3;
	for (i = 0; i < 0x40; i++){
		*(d++) = *(s1--)**(--listFloatp);
	}
	for (i = 0; i < 0x40; i++){
		*(d++) = *(--listFloatp)**(++s1);
	}
	assert(s1 == &wav2[0x3F]);
	assert(s2 == &wav3[0x80]);
	assert(d == &wav3[0x80]);
}

void clHCA::stChannel::Encode8(int32_t index)
{
	const fm_t *listFloat = floatList8[0];
	const fm_t *d = &wav3[0x80];
	fm_t *s2 = &wav3[0x80];
	fm_t *s1 = &wav2[0x3F];
	int32_t i;
	for (i = 0x40; i > 0; i--){
		*(s1--) = (*(--d))/(*(listFloat++));
	}
	for (i = 0x40; i > 0; i--){
		*(++s1) = (*(--d))/(*(listFloat++));
	}
	assert(d == wav3);
	d = &wave[index][0x80];
	s1++;
	for (i = 0x40; i > 0; i--){
		*(--s2) = ((*(s1++))**(--listFloat)) - *(--d);
	}
	for (i = 0x40; i > 0; i--){
		*(--s2) = ((*(--s1))**(--listFloat)) + *(--d);
	}
	assert(s1 == &wav2[0x40]);
	assert(s2 == wav3);
	assert(d == &wave[index][0]);
	assert(listFloat = floatList8[0]);
}

//--------------------------------------------------
// チェックサム
//--------------------------------------------------
uint16_t clHCA::CheckSum(void *data, int32_t size, uint16_t sum){
	static uint16_t value[] = {
		0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
		0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
		0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
		0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
		0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
		0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
		0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
		0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132, 0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
		0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
		0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
		0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
		0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
		0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
		0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
		0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252, 0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
		0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202,
	};
	for (uint8_t *s = (uint8_t *)data, *e = s + size; s < e; s++){
		sum = (sum << 8) ^ value[(sum >> 8) ^ *s];
	}
	return sum;
}
