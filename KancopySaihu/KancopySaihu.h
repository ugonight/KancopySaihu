#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_KancopySaihu.h"

class Record;
class Analysis;

// メイン画面
class KancopySaihu : public QMainWindow
{
	Q_OBJECT

public:
	KancopySaihu(QWidget *parent = Q_NULLPTR);

	void setFileName(QString str);
	void setTempo(double tempo);

	int getRangeL();
	int getRangeH();
	int getQuantize();
	int getTempo();
	double getOffset();
	double getThreshold();

private:
	Ui::KancopySaihuClass ui;
	
	Record *mRecordWindow;
	Analysis *mAnalysisWindow;

private slots:
	void fileReference();	// ファイルを開くダイアログを開く
	void showRecord();		// 録音画面を表示
	void showAnalysis();	// 解析結果画面を表示
	void aboutQt();
};
