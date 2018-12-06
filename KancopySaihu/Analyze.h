#pragma once

#include <QObject>
#include <fftw3.h>

#include <qpixmap.h>
#include "KancopySaihu.h"

// 音声解析本体のクラス

class waveRW;
// class KancopySaihu;

enum AStatus {
	STATUS_NONE,
	STATUS_READY,
	STATUS_FINISH_INIT,
	STATUS_PROCESS_INIT,
	STATUS_FINISH_CREATEPIX,
	STATUS_PROCESS_CREATEPIX
};

class Analyze : public QObject
{
	Q_OBJECT

public:
	Analyze(QObject *parent = 0);
	~Analyze();

public slots:
	void init(QString filename);
	void setMain(KancopySaihu *k);
	double* getData();
	long int getSampleLength();
	float** getFFTWResult(int *i = 0, int *j = 0);	// FFTWの結果 引数にアドレスを渡すと二次元配列の添字([i][j])を返す
	AStatus getStatus();
	QString getStatusMsg();
	void createPixmap(int scale, std::vector<QPixmap> *wave, std::vector<QPixmap> *spect, std::vector<QPixmap> *pitch);

private:
	static const int fftsize;	// バッファサイズ
	static const float dt;		// fftを行う間隔（時間）

	waveRW *mWaveRW;
	KancopySaihu *mMain;
	AStatus mStatus;
	QString mStatusMsg;

	fftw_complex *mFFTW_In;	// 音声データ
	fftw_complex *mFFTW_Out;		// FFTW出力
	fftw_plan mFFTW_Plan;		// FFTWプラン
	float **mFFTW_Result;	// 解析結果（振幅）
	float *mFFTW_Pitch;	// 基本周波数
};
