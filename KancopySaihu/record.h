#pragma once

#include <QWidget>
#include "ui_record.h"

class Record : public QWidget
{
	Q_OBJECT

public:
	Record(QWidget *parent = Q_NULLPTR);
	~Record();

private:
	Ui::record ui;

protected:
	void paintEvent(QPaintEvent *);

private slots:
	void fileReference();	// ファイル保存ダイアログを開く
};
