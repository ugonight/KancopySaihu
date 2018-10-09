#include"wave.h"

#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

waveRW::waveRW() {
	riff_chunk_ID[0] = 'R';
	riff_chunk_ID[1] = 'I';
	riff_chunk_ID[2] = 'F';
	riff_chunk_ID[3] = 'F';
	// riff_chunk_size = 36 + pcm->length * 2;
	file_format_type[0] = 'W';
	file_format_type[1] = 'A';
	file_format_type[2] = 'V';
	file_format_type[3] = 'E';

	fmt_chunk_ID[0] = 'f';
	fmt_chunk_ID[1] = 'm';
	fmt_chunk_ID[2] = 't';
	fmt_chunk_ID[3] = ' ';
	fmt_chunk_size = 16;
	wave_format_type = 1;
	channel = 1;
	//samples_per_sec = pcm->fs; /* 標本化周波数 */
	//bytes_per_sec = pcm->fs * pcm->bits / 8;
	//block_size = pcm->bits / 8;
	//bits_per_sample = pcm->bits; /* 量子化精度 */

	data_chunk_ID[0] = 'd';
	data_chunk_ID[1] = 'a';
	data_chunk_ID[2] = 't';
	data_chunk_ID[3] = 'a';
	//data_chunk_size = pcm->length * 2;
}

waveRW::~waveRW() {
	delete[] s;
	delete[] sL;
	delete[] sR;
}

void waveRW::wave_read(char *file_name) {
	int n;
	short data;

	ifstream fin(file_name, ios::in | ios::binary);

	if (!fin) {
		cout << "ファイルが開けません";
		return;
	}

	//パラメータの取得
	fin.read(riff_chunk_ID, 1 * 4); riff_chunk_ID[4] = '\0';
	fin.read((char*)&riff_chunk_size, 4 * 1);
	fin.read((char*)file_format_type, 1 * 4); file_format_type[4] = '\0';
	fin.read((char*)fmt_chunk_ID, 1 * 4); fmt_chunk_ID[4] = '\0';
	fin.read((char*)&fmt_chunk_size, 4 * 1);
	fin.read((char*)&wave_format_type, 2 * 1);
	fin.read((char*)&channel, 2 * 1);
	fin.read((char*)&samples_per_sec, 4 * 1);
	fin.read((char*)&bytes_per_sec, 4 * 1);
	fin.read((char*)&block_size, 2 * 1);
	fin.read((char*)&bits_per_sample, 2 * 1); 
	fin.read((char*)data_chunk_ID, 1 * 4); data_chunk_ID[4] = '\0';
	fin.read((char*)&data_chunk_size, 4 * 1);

	// データの取得
	fs = samples_per_sec; /* 標本化周波数 */
	bits = bits_per_sample; /* 量子化精度 */
	length = data_chunk_size / block_size; /* 音データの長さ */
	s = new double[length]; /* メモリの確保 */
	sL = new double[length]; 
	sR = new double[length];

	for (n = 0; n < length; n++)
	{
		if (channel == 1) {
			fin.read((char*)&data, 2 * 1); /* 音データの読み取り */
			s[n] = NormalizationData(data);
		}
		else if (channel == 2) {
			fin.read((char*)&data, 2 * 1); /* 音データ（Lチャンネル）の読み取り */
			sL[n] = NormalizationData(data);

			fin.read((char*)&data, 2 * 1); /* 音データ（Rチャンネル）の読み取り */
			sR[n] = NormalizationData(data);

			s[n] =  sL[n] + sR[n];	// 単純に足し合わせてモノラル変換
		}

	}

	fin.close();
}

void waveRW::wave_write(char *file_name) {
	FILE *fp;

	fopen_s(&fp, file_name, "wb");

	fwrite(riff_chunk_ID, 1, 4, fp);
	fwrite(&riff_chunk_size, 4, 1, fp);
	fwrite(file_format_type, 1, 4, fp);
	fwrite(fmt_chunk_ID, 1, 4, fp);
	fwrite(&fmt_chunk_size, 4, 1, fp);
	fwrite(&wave_format_type, 2, 1, fp);
	fwrite(&channel, 2, 1, fp);
	fwrite(&samples_per_sec, 4, 1, fp);
	fwrite(&bytes_per_sec, 4, 1, fp);
	fwrite(&block_size, 2, 1, fp);
	fwrite(&bits_per_sample, 2, 1, fp);
	fwrite(data_chunk_ID, 1, 4, fp);
	fwrite(&data_chunk_size, 4, 1, fp);

	double ss, ssL,ssR, ampl = (bits_per_sample == 8) ? 256.0 : 65536.0;
	short data;
	
	if (channel == 1) {

		for (int n = 0; n < length; n++)
		{
			ss = (s[n] + 1.0) / 2.0 * ampl;

			if (ss > ampl - 1.0)
			{
				ss = ampl - 1.0; /* クリッピング */
			}
			else if (ss < 0.0)
			{
				ss = 0.0; /* クリッピング */
			}

			if (bits_per_sample == 8) {
				data = (short)((int)(ss + 0.5)); /* 四捨五入 */

			}
			else {
				data = (short)((int)(ss + 0.5) - ampl / 2); /* 四捨五入とオフセットの調節 */
			}

			fwrite(&data, 2, 1, fp); /* 音データの書き出し */
		}
	}
	else {
		for (int n = 0; n < length; n++)
		{
			ssL = (sL[n] + 1.0) / 2.0 * ampl;

			if (ssL > ampl -1.0)
			{
				ssL = ampl -1.0; /* クリッピング */
			}
			else if (ssL < 0.0)
			{
				ssL = 0.0; /* クリッピング */
			}

			if (bits_per_sample == 8) {
				data = (short)((int)(ssL + 0.5)); 

			}
			else {
				data = (short)((int)(ssL + 0.5) - ampl / 2);
			}

			fwrite(&data, 1, 1, fp); /* 音データ（Lチャンネル）の書き出し */

			ssR = (sR[n] + 1.0) / 2.0 * ampl;

			if (ssR > ampl-1.0)
			{
				ssR = ampl-1; /* クリッピング */
			}
			else if (ssR < 0.0)
			{
				ssR = 0.0; /* クリッピング */
			}

			if (bits_per_sample == 8) {
				data = (short)((int)(ssR + 0.5));

			}
			else {
				data = (short)((int)(ssR + 0.5) - ampl / 2);
			}
			fwrite(&data, 1, 1, fp); /* 音データ（Rチャンネル）の書き出し */
		}
	}

	fclose(fp);
}


double	waveRW::NormalizationData(double data) {
	double result = 0.0;
	
	/* 音データを-1以上1未満の範囲に正規化する */
	if (bits_per_sample == 8) {
		result = ((double)data - 128.0) / 128.0;
	}
	else if (bits_per_sample == 16) {
		result = (double)data / 32768.0;
	}
	else {
		result = -1.0;
	}

	return result;
}

char* waveRW::getRiffChunkID() { return riff_chunk_ID; }
long waveRW::getRiffChunkSize() { return riff_chunk_size; }
char* waveRW::getFileFormatType() { return file_format_type; }
char* waveRW::getFmtChunkID() { return fmt_chunk_ID; }
long waveRW::getFmtChunkSize() { return fmt_chunk_size; }
short waveRW::getWaveFormatType() { return wave_format_type; }
short waveRW::getChannel() { return channel; }
long waveRW::getSamplesPerSec() { return samples_per_sec; }
long waveRW::getBytesPerSec() { return bytes_per_sec; }
short waveRW::getBlockSize() { return block_size; }
short waveRW::getBitsPerSample() { return bits_per_sample; }
char* waveRW::getDataChunkID() { return data_chunk_ID; }
long waveRW::getDataChunkSize() { return data_chunk_size; }
double* waveRW::getData() { return s; }
double* waveRW::getDataL() { return sL; }
double* waveRW::getDataR() { return sR; }
long waveRW::getLength() { return length; }

void waveRW::setRiffChunkID(char* c) { strcpy_s(riff_chunk_ID, 4, c); }
void waveRW::setRiffChunkSize(long l) { riff_chunk_size = l; }
void waveRW::setFileFormatType(char* c) { strcpy_s(file_format_type, 4, c); }
void waveRW::setFmtChunkID(char* c) { strcpy_s(fmt_chunk_ID, 4, c); }
void waveRW::setFmtChunkSize(long l) { fmt_chunk_size = l; }
void waveRW::setWaveFormatType(short s) { wave_format_type = s; }
void waveRW::setChannel(short s) { channel = s; }
void waveRW::setSamplesPerSec(long l) { samples_per_sec = l; }
void waveRW::setBytesPerSec(long l) { bytes_per_sec = l; }
void waveRW::setBlockSize(short s) { block_size = s; }
void waveRW::setBitsPerSample(short s) { bits_per_sample = s; }
void waveRW::setDataChunkID(char* c) { strcpy_s(data_chunk_ID, 4, c); }
void waveRW::setDataChunkSize(long l) { data_chunk_size = l; }
void waveRW::setData(double* d) { delete[] s; s = new double[sizeof(d) / sizeof(double)]; memcpy(s, d, sizeof(d)); }
void waveRW::setDataL(double* d) { delete[] sL; sL = new double[sizeof(d) / sizeof(double)]; memcpy(sL, d, sizeof(d)); }
void waveRW::setDataR(double* d) { delete[] sR; sR = new double[sizeof(d) / sizeof(double)]; memcpy(sR, d, sizeof(d)); }
void waveRW::setLength(long l) { length = l; }