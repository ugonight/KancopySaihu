#pragma once

#include <QObject>
#include <fftw3.h>

// ������͖{�̂̃N���X

class waveRW;

class Analyze : public QObject
{
	Q_OBJECT

public:
	Analyze(QObject *parent);
	~Analyze();

	void init(QString filename);
	double* getData();
	long int getSampleLength();
	float** getFFTWResult(int *i = 0, int *j = 0);	// FFTW�̌��� �����ɃA�h���X��n���Ɠ񎟌��z��̓Y��([i][j])��Ԃ�
private:
	const int fftsize = 1024;	// �o�b�t�@�T�C�Y
	const float dt = 0.001;		// 1ms�b

	waveRW *mWaveRW;

	fftw_complex *mFFTW_In;	// �����f�[�^
	fftw_complex *mFFTW_Out;		// FFTW�o��
	fftw_plan mFFTW_Plan;		// FFTW�v����
	float **mFFTW_Result;	// ��͌��ʁi�U���j
};
