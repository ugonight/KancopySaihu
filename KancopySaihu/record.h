#pragma once

#include <QWidget>
#include <qstatusbar.h>
#include "ui_record.h"
#include <vector>

#include <fftw3.h>
#include <portaudio.h>

class JuliusT;

class Record : public QWidget
{
	Q_OBJECT

public:
	Record(QWidget *parent = Q_NULLPTR);
	~Record();

	void setParent(QWidget *parent);

private:
	Ui::record ui;

	struct dWave {
		double data;
		bool rec;
	};

	// PortAudio オーディオ処理コールバック関数 
	static int Record::dsp(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
	void getWord();

	QStatusBar *mStatusBar;

	PaStream *mStream;		// PortAudioストリーム
	fftw_complex *mData;	// 音声データ
	fftw_complex *mOut;		// FFTW出力
	fftw_plan mPlan;		// FFTWプラン
	JuliusT *mJuliusT;

	static std::vector<double> mRecData;	// 録音データ
	static std::vector<dWave> mWaveData;
	std::vector<QString> mWordData;

	int mDispMode;
	static bool mRecNow;
	QWidget *mParent;		// 親ウィンドウ
protected:
	void paintEvent(QPaintEvent *);

private slots:
	void fileReference();	// ファイル保存ダイアログを開く
	void changeDispMode();	// 表示変更
	void rec();				// 録音ボタンが押された
};
