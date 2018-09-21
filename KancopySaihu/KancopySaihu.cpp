#pragma execution_character_set("utf-8")

#include <QtWidgets>
#include "KancopySaihu.h"
#include "record.h"

KancopySaihu::KancopySaihu(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
}

void KancopySaihu::fileReference() {
	QString selFilter = tr("音声ファイル(*.wav)");
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("ファイルを開く"),
		"",
		tr("すべて(*.*);;音声ファイル(*.wav)"),
		&selFilter,
		QFileDialog::DontUseCustomDirectoryIcons
	);
	if (fileName.isEmpty()) {
		// キャンセル
		return;
	}
	ui.lineFileName->setText(fileName);
}

void KancopySaihu::showRecord() {
	mRecordWindow = new Record();
	mRecordWindow->show();
}