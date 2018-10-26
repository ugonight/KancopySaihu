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
	void fileReference();	// �t�@�C�����J���_�C�A���O���J��
	void showRecord();		// �^����ʂ�\��
};
