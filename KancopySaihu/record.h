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

	// PortAudio �I�[�f�B�I�����R�[���o�b�N�֐� 
	static int Record::dsp(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

	PaStream *mStream;		// PortAudio�X�g���[��
	fftw_complex *mData;	// �����f�[�^
	fftw_complex *mOut;		// FFTW�o��
	fftw_plan mPlan;		// FFTW�v����

	static std::vector<float> mRecData;	// �^���f�[�^
	static std::vector<float> mWaveData;

	int mDispMode;
	static bool mRecNow;
protected:
	void paintEvent(QPaintEvent *);

private slots:
	void fileReference();	// �t�@�C���ۑ��_�C�A���O���J��
	void changeDispMode();	// �\���ύX
	void rec();				// �^���{�^���������ꂽ
};
