#pragma once

#include <QWidget>
#include "ui_analysis.h"

class Analyze;

// ‰ğÍŒ‹‰Ê‰æ–Ê
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
	
	double mScaleX;	// ‰¡‚ÌkÚB1/x‚Å•\‚·B

	Analyze *mAnalyze;
	QString mFileName;
	double *mWaveData;
	long int mWaveLength;
	float **mFFTWData;
	int mFFTnum, mFFTsize;

private slots:
	void scaleUp();
	void scaleDown();
	void sliderChange();
	void wSizeChanged();
};
