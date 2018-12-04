#pragma execution_character_set("utf-8")

#include "analysis.h"
#include "Analyze.h"
#include"KancopySaihu.h"

#include<qpainter.h>
#include<qtimer.h>
#include<qthread.h>
#include <qdebug.h>


Analysis::Analysis(QWidget *parent)
	: QWidget(parent), mAnalyze(0),mWaveData(0),mScaleX(10), mFFTWData(0)
{
	ui.setupUi(this);

	mAnalyze = new Analyze(this);
	mThread = new QThread;
	mAnalyze->setParent(NULL);
	mAnalyze->moveToThread(mThread);
	mThread->start();

	// タイマー
	auto timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(1000 / 30);	// 30fps
}

Analysis::~Analysis()
{
	delete mAnalyze;
	mThread->quit();
	mThread->wait();
}

void Analysis::setMain(KancopySaihu *parent) {
	mParent = parent;
}

void Analysis::update() {
	AStatus status;
	QMetaObject::invokeMethod(mAnalyze, "getStatus", Qt::DirectConnection, Q_RETURN_ARG(AStatus, status));	// status = mAnalyze->getStatus();

	switch (status)
	{
	case STATUS_NONE:
		break;
	case STATUS_READY:
		break;
	case STATUS_FINISH_INIT:
		QMetaObject::invokeMethod(mAnalyze, "getData", Qt::DirectConnection, Q_RETURN_ARG(double*, mWaveData));	// mWaveData = mAnalyze->getData();
		QMetaObject::invokeMethod(mAnalyze, "getSampleLength", Qt::DirectConnection, Q_RETURN_ARG(long int, mWaveLength));	// mWaveLength = mAnalyze->getSampleLength();
		QMetaObject::invokeMethod(mAnalyze, "getFFTWResult", Qt::DirectConnection, Q_RETURN_ARG(float**, mFFTWData), Q_ARG(int*, &mFFTnum), Q_ARG(int*, &mFFTsize));	// mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

		createPixmap();	// 画像データ生成

		break;
	case STATUS_FINISH_CREATEPIX:
		wSizeChanged();	// スクロールバーの初期化

		break;
	default:
		break;
	}

	// ステータス文字列取得
	QString msg = "";
	QMetaObject::invokeMethod(mAnalyze, "getStatusMsg", Qt::DirectConnection, Q_RETURN_ARG(QString, msg));	// msg = mAnalyze->getStatusMsg();
	msg = QString("ステータス:%1").arg(msg);
	if (ui.labelStatus->text() != msg)
		ui.labelStatus->setText(msg);

}

void Analysis::paintEvent(QPaintEvent *) {
	
	int w = ui.openGLWidget->width(), h = ui.openGLWidget->height();

	//// ペンの準備
	QPainter painter(ui.openGLWidget);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));

	// キャンバスの初期化
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);
	painter.end();

	int scrollX = ui.horizontalScrollBar->value(), count = 0, tsize, x, sx, sw;
	std::vector<std::vector<QPixmap>> lists;
	lists.push_back(mWavePix);
	lists.push_back(mSpectPix);
	lists.push_back(mPitchPix);

	for (auto list : lists) {
		tsize = 0;
		for (auto pix : list) {
			auto pix_ = pix.scaledToHeight(h / 3);
			auto rect1 = QRect(tsize, 0, pix_.width(), pix_.height());	// pixmapの領域
			auto rect2 = QRect(scrollX, 0, w, h / 3);	// 表示領域

			if (rect1.intersects(rect2)) {	// 表示領域に入っていたら
				QPainter p(ui.openGLWidget);

				x = (rect1.x() > rect2.x()) ? rect1.x() - rect2.x() : 0;
				sx = (rect1.x() < rect2.x()) ? rect2.x() - rect1.x() : 0;
				sw = rect1.intersected(rect2).width(); // (rect2.x() < rect1.right()) ? rect1.right() - rect2.x() : (rect2.right() > rect1.x()) ? rect2.right() - rect1.x() : rect1.width();
				
				p.drawPixmap(x, (h / 3)*count, pix_, sx, 0, sw, h / 3);
				
				p.end();
			}
			tsize += pix_.width();
		}
		count++;
	}
}

void Analysis::analyze(QString filename) {

	qDebug() << QThread::currentThreadId();
	QMetaObject::invokeMethod(mAnalyze, "init", Qt::QueuedConnection, Q_ARG(QString,filename));	// mAnalyze->init(filename);
	QMetaObject::invokeMethod(mAnalyze, "setMain", Qt::QueuedConnection, Q_ARG(KancopySaihu*, mParent));	// mAnalyze->setMain(mParent);
	//mWaveData = mAnalyze->getData();
	//mWaveLength = mAnalyze->getSampleLength();
	//mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

	//createPixmap();	// 画像データ生成
	//wSizeChanged();	// スクロールバーの初期化
}

void Analysis::scaleUp() { if (mScaleX > 0) mScaleX -= 10; if (mScaleX == 0) mScaleX = 1; createPixmap(); wSizeChanged(); paintEvent(NULL); }
void Analysis::scaleDown() { mScaleX += 10; createPixmap(); wSizeChanged(); paintEvent(NULL); }
void Analysis::sliderChange() {  paintEvent(NULL); }
void Analysis::wSizeChanged() { 
	float ratio = (float)ui.horizontalScrollBar->value() / (float)ui.horizontalScrollBar->maximum();

	int ImageW = 0; // = mWavePix.scaledToHeight(ui.openGLWidget->height() / 2).width();
	for (auto pix : mWavePix) {
		ImageW += pix.scaledToHeight(ui.openGLWidget->height() / 3).width();
	}

	if (ImageW > ui.openGLWidget->width()) {
		ui.horizontalScrollBar->setMaximum(ImageW - ui.openGLWidget->width());
	}
	else {
		ui.horizontalScrollBar->setMaximum(0);
	}

	ui.horizontalScrollBar->setValue(ui.horizontalScrollBar->maximum() * ratio);

	// qDebug("%d", ui.horizontalScrollBar->value());
}	// スクロールバーの初期化


void Analysis::createPixmap() {
	mWavePix.clear();
	mSpectPix.clear();
	mPitchPix.clear();

	qRegisterMetaType<std::vector<QPixmap>*>("std::vector<QPixmap>*");
	QMetaObject::invokeMethod(mAnalyze, "createPixmap", Qt::QueuedConnection, Q_ARG(int, mScaleX),Q_ARG(std::vector<QPixmap>*,&mWavePix), Q_ARG(std::vector<QPixmap>*, &mSpectPix), Q_ARG(std::vector<QPixmap>*, &mPitchPix)); // mAnalyze->createPixmap(mScaleX, &mWavePix, &mSpectPix, &mPitchPix);
}