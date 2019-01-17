#pragma once

#include <QObject>
#include <fftw3.h>

#include <qpixmap.h>
#include "KancopySaihu.h"
#include "JuliusT.h"

// 音声解析本体のクラス

class waveRW;
// class KancopySaihu;
class JuliusT;

enum AStatus {
	STATUS_NONE,
	STATUS_READY,
	STATUS_FINISH_INIT,
	STATUS_PROCESS_INIT,
	STATUS_FINISH_CREATEPIX,
	STATUS_PROCESS_CREATEPIX,
	STATUS_FINISH_JULIUS_FIRST,
	STATUS_FINISH_CREATEPIX_MFCC,
	STATUS_PROCESS_CREATEPIX_MFCC
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
	int getStatusP(int id);
	void createPixmap(int scale, std::vector<QPixmap> *wave, std::vector<QPixmap> *spect, std::vector<QPixmap> *pitch);
	void createPixmapMfcc(int scale, std::vector<QPixmap> *mfcc);	// mfccのみの描画

private:
	static const int fftsize;	// バッファサイズ
	static const float dt;		// fftを行う間隔（時間）

	waveRW *mWaveRW;
	KancopySaihu *mMain;
	JuliusT *mJulius;
	AStatus mStatus;
	QString mStatusMsg; int mStatusP[2];
	QThread *mThread;

	fftw_complex *mFFTW_In;	// 音声データ
	fftw_complex *mFFTW_Out;		// FFTW出力
	fftw_plan mFFTW_Plan;		// FFTWプラン
	float **mFFTW_Result;	// 解析結果（振幅）
	float *mFFTW_Pitch;	// 基本周波数

	mfcc_tuple mMfccResult;
	std::vector<float> mTimingResult;

	void analyzeTiming();	// タイミング解析
	void analyzeLyrics();	// 歌詞解析

protected:
	void timerEvent(QTimerEvent *event) override;
};
