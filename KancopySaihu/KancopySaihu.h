#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_KancopySaihu.h"

class Record;

class KancopySaihu : public QMainWindow
{
	Q_OBJECT

public:
	KancopySaihu(QWidget *parent = Q_NULLPTR);

	void setFileName(QString str);
	void setTempo(double tempo);

private:
	Ui::KancopySaihuClass ui;
	
	Record *mRecordWindow;

private slots:
	void fileReference();	// ファイルを開くダイアログを開く
	void showRecord();		// 録音画面を表示
};
