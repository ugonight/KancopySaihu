#pragma once

#include <QWidget>
#include "ui_record.h"
#include <vector>

#include <fftw3.h>
#include <portaudio.h>

class Record : public QWidget
{
	Q_OBJECT

public:
	Record(QWidget *parent = Q_NULLPTR);
	~Record();

	//virtual void update();

private:
	Ui::record ui;

	// PortAudio オーディオ処理コールバック関数 
	static int Record::dsp(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

	PaStream *mStream;		// PortAudioストリーム
	fftw_complex *mData;	// 音声データ
	fftw_complex *mOut;		// FFTW出力
	fftw_plan mPlan;		// FFTWプラン

	static std::vector<float> mRecData;	// 録音データ
	static std::vector<float> mWaveData;

	int mDispMode;
	static bool mRecNow;
protected:
	void paintEvent(QPaintEvent *);

private slots:
	void fileReference();	// ファイル保存ダイアログを開く
	void changeDispMode();	// 表示変更
	void rec();				// 録音ボタンが押された
};
