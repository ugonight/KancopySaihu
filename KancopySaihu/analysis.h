#pragma once

#include <QWidget>
#include "ui_analysis.h"

class Analyze;

// âêÕåãâ âÊñ 
class Analysis : public QWidget
{
	Q_OBJECT

public:
	Analysis(QWidget *parent = Q_NULLPTR);
	~Analysis();

	void analyze(QString filename);

protected:
	void paintEvent(QPaintEvent *);

private:
	Ui::Analysis ui;
	
	double mScaleX;	// â°ÇÃèké⁄ÅB1/xÇ≈ï\Ç∑ÅB

	Analyze *mAnalyze;
	QString mFileName;
	double *mWaveData;
	long int mWaveLength;
	float **mFFTWData;
	int mFFTnum, mFFTsize;

	QPixmap mWavePix, mSpectPix;

	void createPixmap();

private slots:
	void scaleUp();
	void scaleDown();
	void sliderChange();
	void wSizeChanged();
};
