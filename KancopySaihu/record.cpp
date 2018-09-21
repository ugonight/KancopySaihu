#pragma execution_character_set("utf-8")

#include "record.h"
#include <QDir>
#include <QtWidgets>

Record::Record(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	// デフォルトの録音の保存先を指定
	QDir curDir(QDir::current());
	QString canonicalDir(curDir.canonicalPath());
	ui.lineFileName->setText(canonicalDir + "\\record.wav");
}

Record::~Record()
{
}

void Record::paintEvent(QPaintEvent *) {
	QPainter painter(ui.openGLWidget);
	painter.setRenderHint(QPainter::Antialiasing, true);//アンチエイリアスセット
	painter.setPen(QPen(Qt::black, 12, Qt::DashDotLine, Qt::RoundCap));
	painter.setBrush(QBrush(Qt::green, Qt::SolidPattern));
	painter.drawEllipse(80, 80, 400, 240);//楕円描画
}


void Record::fileReference() {
	QString selFilter;
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("名前を付けて保存"),
		".",
		tr("音声ファイル(*.wav)"),
		&selFilter,
		QFileDialog::DontUseCustomDirectoryIcons
	);
	if (fileName.isEmpty()) {
		return;
	}
	ui.lineFileName->setText(fileName);
}
