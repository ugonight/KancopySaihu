#pragma once

// wave�t�@�C����ǂݏ�������N���X
// �Q�l: �T�E���h�v���O���~���O����

class waveRW {
private:
	int fs; /* �W�{�����g�� */
	int bits; /* �ʎq�����x */
	int length; /* ���f�[�^�̒��� */
	double *s; /* ���f�[�^ */
	double *sL; /* ���f�[�^�iL�`�����l���j */
	double *sR; /* ���f�[�^�iR�`�����l���j */

	char riff_chunk_ID[4 + 1];
	long riff_chunk_size;
	char file_format_type[4 + 1];
	char fmt_chunk_ID[4 + 1];
	long fmt_chunk_size;
	short wave_format_type;
	short channel;
	long samples_per_sec;
	long bytes_per_sec;
	short block_size;
	short bits_per_sample;
	char data_chunk_ID[4 + 1];
	long data_chunk_size;
	//short data;

	// �f�[�^�̐��K��
	double	NormalizationData(double data);
public:
	waveRW();
	~waveRW();

	void wave_read(char *file_name);
	void wave_write(char *file_name);


	// �l�擾
	char* getRiffChunkID();
	long getRiffChunkSize();
	char* getFileFormatType();
	char* getFmtChunkID();
	long getFmtChunkSize();
	short getWaveFormatType();
	short getChannel();
	long getSamplesPerSec();
	long getBytesPerSec();
	short getBlockSize();
	short getBitsPerSample();
	char* getDataChunkID();
	long getDataChunkSize();
	double* getData();
	double* getDataL();
	double* getDataR();
	long getLength();

	// �l�ݒ�
	void setRiffChunkID(char*);
	void setRiffChunkSize(long);
	void setFileFormatType(char*);
	void setFmtChunkID(char*);
	void setFmtChunkSize(long);
	void setWaveFormatType(short);
	void setChannel(short);
	void setSamplesPerSec(long);
	void setBytesPerSec(long);
	void setBlockSize(short);
	void setBitsPerSample(short);
	void setDataChunkID(char*);
	void setDataChunkSize(long);
	void setData(double*);
	void setDataL(double*);
	void setDataR(double*);
	void setLength(long);

};