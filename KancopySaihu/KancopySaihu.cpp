#pragma execution_character_set("utf-8")

#include <QtWidgets>
#include "KancopySaihu.h"
#include "record.h"
#include "analysis.h"


KancopySaihu::KancopySaihu(QWidget *parent)
	: QMainWindow(parent), mRecordWindow(0), mAnalysisWindow(0)
{
	ui.setupUi(this);

	// 項目の追加
	ui.comboBoxQuantize->addItems(tr("2,3,4,6,8,12,16,24,32,48,64").split(","));
	ui.comboBoxQuantize->setCurrentIndex(4);
	QString pitch[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
	for (int i = 1; i <= 7; i++) {					// C1～B7
		for (int j = 0; j < 12; j++) {
			ui.comboBoxRangeL->addItem(pitch[j] + QString::number(i));
			ui.comboBoxRangeH->addItem(pitch[j] + QString::number(i));
		}
	}
	ui.comboBoxRangeL->setCurrentIndex(12);			// C2
	ui.comboBoxRangeH->setCurrentIndex(12 * 5 - 1);	// B5
}

void KancopySaihu::setFileName(QString str) {
	ui.lineFileName->setText(str);
}
void KancopySaihu::setTempo(double tempo) {
	ui.labelTempo->setText(QString::number(tempo));
}


int KancopySaihu::getRangeL() { return ui.comboBoxRangeL->currentIndex(); }
int KancopySaihu::getRangeH() { return ui.comboBoxRangeH->currentIndex(); }
int KancopySaihu::getQuantize() { return ui.comboBoxQuantize->currentText().toInt(); }
int KancopySaihu::getTempo() { return ui.labelTempo->text().toInt(); }
double KancopySaihu::getOffset(){return ui.doubleSpinBoxOffset->value(); }
double KancopySaihu::getThreshold() { return ui.doubleSpinBoxThreshold->value(); }


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
	if (!mRecordWindow) {
		mRecordWindow = new Record();
		mRecordWindow->show();
		mRecordWindow->setParent(this);
	}else
	if (mRecordWindow->isHidden()) {
		mRecordWindow->show();
	}
}

void KancopySaihu::showAnalysis() {
	if (ui.lineFileName->text() == "") {
		QMessageBox::information(this, tr(""), tr("ファイル名を入力してください。")); 
		return;
	}

	if (!mAnalysisWindow) {
		mAnalysisWindow = new Analysis();
		mAnalysisWindow->setMain(this);
		mAnalysisWindow->analyze(ui.lineFileName->text());
		mAnalysisWindow->show();
	}
	else if (mAnalysisWindow->isHidden())
	{
		mAnalysisWindow->analyze(ui.lineFileName->text());
		mAnalysisWindow->show();
	}
}

void KancopySaihu::aboutQt() {
	QApplication::aboutQt();
}