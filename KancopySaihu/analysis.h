#pragma once

#include <QWidget>
#include "ui_analysis.h"

class Analyze;
class KancopySaihu;

// ��͌��ʉ��
class Analysis : public QWidget
{
	Q_OBJECT

public:
	Analysis(QWidget *parent = Q_NULLPTR);
	~Analysis();

	void analyze(QString filename);
	void setMain(KancopySaihu *parent);

protected:
	void paintEvent(QPaintEvent *);

private:
	Ui::Analysis ui;
	
	double mScaleX;	// ���̏k�ځB1/x�ŕ\���B

	Analyze *mAnalyze;
	KancopySaihu *mParent;
	QString mFileName;
	double *mWaveData;
	long int mWaveLength;
	float **mFFTWData;
	int mFFTnum, mFFTsize;

	// QPixmap mWavePix, mSpectPix;
	std::vector<QPixmap> mWavePix, mSpectPix, mPitchPix;

	void createPixmap();

private slots:
	void scaleUp();
	void scaleDown();
	void sliderChange();
	void wSizeChanged();
};
