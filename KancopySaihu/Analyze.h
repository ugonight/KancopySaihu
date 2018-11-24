#pragma once

#include <QObject>
#include <fftw3.h>

// 音声解析本体のクラス

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
	float** getFFTWResult(int *i = 0, int *j = 0);	// FFTWの結果 引数にアドレスを渡すと二次元配列の添字([i][j])を返す
private:
	const int fftsize = 1024;	// バッファサイズ
	const float dt = 0.001;		// 1ms秒

	waveRW *mWaveRW;

	fftw_complex *mFFTW_In;	// 音声データ
	fftw_complex *mFFTW_Out;		// FFTW出力
	fftw_plan mFFTW_Plan;		// FFTWプラン
	float **mFFTW_Result;	// 解析結果（振幅）
};
