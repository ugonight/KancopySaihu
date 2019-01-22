#pragma once

#include <QWidget>
#include "ui_analysis.h"

class Analyze;
class KancopySaihu;

// 解析結果画面
class Analysis : public QWidget
{
	Q_OBJECT

public:
	Analysis(QWidget *parent = Q_NULLPTR);
	~Analysis();

	void analyze(QString filename);
	void setMain(KancopySaihu *parent);

public slots:
	void update();

protected:
	void paintEvent(QPaintEvent *);

private:
	Ui::Analysis ui;
	
	double mScaleX;	// 横の縮尺。1/xで表す。

	Analyze *mAnalyze;
	KancopySaihu *mParent;
	QString mFileName;
	double *mWaveData;
	long int mWaveLength;
	float **mFFTWData;
	int mFFTnum, mFFTsize;
	QThread *mThread;
	int mScaleChangeTime;
	int mCreatedPix;

	// QPixmap mWavePix, mSpectPix;
	std::vector<QPixmap> mWavePix, mSpectPix, mPitchPix, mMfccPix/*, mLyricsPix*/;
	std::vector<std::pair<float, QString>> mTimingLyricsList;	// タイミングと歌詞のペアのリスト

	void createPixmap();

private slots:
	void scaleUp();
	void scaleDown();
	void sliderChange();
	void wSizeChanged();
	void reAnalyze();
	void confirm();
};
