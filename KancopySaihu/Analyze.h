#pragma once

#include <QObject>
#include <fftw3.h>

#include <qpixmap.h>
#include "KancopySaihu.h"

// 音声解析本体のクラス

class waveRW;
// class KancopySaihu;

class Analyze : public QObject
{
	Q_OBJECT

public:
	Analyze(QObject *parent = 0);
	~Analyze();

	void init(QString filename);
	void setMain(KancopySaihu *k);
	double* getData();
	long int getSampleLength();
	float** getFFTWResult(int *i = 0, int *j = 0);	// FFTWの結果 引数にアドレスを渡すと二次元配列の添字([i][j])を返す
	void createPixmap(int scale, std::vector<QPixmap> *wave, std::vector<QPixmap> *spect, std::vector<QPixmap> *pitch);

private:
	static const int fftsize;	// バッファサイズ
	static const float dt;		// fftを行う間隔（時間）

	waveRW *mWaveRW;
	KancopySaihu *mMain;

	fftw_complex *mFFTW_In;	// 音声データ
	fftw_complex *mFFTW_Out;		// FFTW出力
	fftw_plan mFFTW_Plan;		// FFTWプラン
	float **mFFTW_Result;	// 解析結果（振幅）
};
