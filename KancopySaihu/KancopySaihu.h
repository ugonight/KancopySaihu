#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_KancopySaihu.h"

class Record;
class Analysis;

// ���C�����
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
	void fileReference();	// �t�@�C�����J���_�C�A���O���J��
	void showRecord();		// �^����ʂ�\��
	void showAnalysis();	// ��͌��ʉ�ʂ�\��
	void aboutQt();
};
