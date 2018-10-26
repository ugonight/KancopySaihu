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

	// PortAudio �I�[�f�B�I�����R�[���o�b�N�֐� 
	static int Record::dsp(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);
	void getWord();

	QStatusBar *mStatusBar;

	PaStream *mStream;		// PortAudio�X�g���[��
	fftw_complex *mData;	// �����f�[�^
	fftw_complex *mOut;		// FFTW�o��
	fftw_plan mPlan;		// FFTW�v����
	JuliusT *mJuliusT;

	static std::vector<double> mRecData;	// �^���f�[�^
	static std::vector<dWave> mWaveData;
	std::vector<QString> mWordData;

	int mDispMode;
	static bool mRecNow;
	QWidget *mParent;		// �e�E�B���h�E
protected:
	void paintEvent(QPaintEvent *);

private slots:
	void fileReference();	// �t�@�C���ۑ��_�C�A���O���J��
	void changeDispMode();	// �\���ύX
	void rec();				// �^���{�^���������ꂽ
};
