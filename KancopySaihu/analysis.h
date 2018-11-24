#pragma once

#include <QWidget>
#include "ui_analysis.h"

class Analyze;

// 解析結果画面
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
	
	double mScaleX;	// 横の縮尺。1/xで表す。

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
