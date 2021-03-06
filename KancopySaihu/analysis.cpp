#pragma execution_character_set("utf-8")

#include "analysis.h"
#include "Analyze.h"
#include "Control.h"
#include"KancopySaihu.h"

#include<qpainter.h>
#include<qtimer.h>
#include<qthread.h>
#include <qdebug.h>
#include<qapplication.h>



Analysis::Analysis(QWidget *parent)
	: QWidget(parent), mAnalyze(0),mWaveData(0),mScaleX(50), mFFTWData(0), mScaleChangeTime(-1),mCreatedPix(0)
{
	ui.setupUi(this);

	//mAnalyze = new Analyze(this);
	//mThread = new QThread;
	//mAnalyze->setParent(NULL);
	//mAnalyze->moveToThread(mThread);
	//mThread->start();

	// タイマー
	auto timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(1000 / 30);	// 30fps
}

Analysis::~Analysis()
{
	delete mAnalyze;
	//mThread->quit();
	//mThread->wait();
	// delete mThread;
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
	case STATUS_PROCESS_INIT:
	case STATUS_PROCESS_CREATEPIX:
		ui.toolScaleUp->setEnabled(false);
		ui.toolScaleDown->setEnabled(false);
		break;
	case STATUS_FINISH_INIT:
		QMetaObject::invokeMethod(mAnalyze, "getData", Qt::DirectConnection, Q_RETURN_ARG(double*, mWaveData));	// mWaveData = mAnalyze->getData();
		QMetaObject::invokeMethod(mAnalyze, "getSampleLength", Qt::DirectConnection, Q_RETURN_ARG(long int, mWaveLength));	// mWaveLength = mAnalyze->getSampleLength();
		QMetaObject::invokeMethod(mAnalyze, "getFFTWResult", Qt::DirectConnection, Q_RETURN_ARG(float**, mFFTWData), Q_ARG(int*, &mFFTnum), Q_ARG(int*, &mFFTsize));	// mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

		createPixmap();	// 画像データ生成

		break;
	case STATUS_FINISH_CREATEPIX:
		if (mCreatedPix == 0) {
			wSizeChanged();	// スクロールバーの初期化
			ui.toolScaleUp->setEnabled(true);
			ui.toolScaleDown->setEnabled(true);
			mCreatedPix++;
		}
		break;
	case STATUS_FINISH_CREATEPIX_MFCC:
		if (mCreatedPix == 1) {
			wSizeChanged();	// スクロールバーの初期化
			ui.toolScaleUp->setEnabled(true);
			ui.toolScaleDown->setEnabled(true);
			mCreatedPix++;
		}	
		break;

	case STATUS_FINISH_JULIUS_FIRST:
		mMfccPix.clear();
		QMetaObject::invokeMethod(mAnalyze, "createPixmapMfcc", Qt::QueuedConnection, Q_ARG(int, mScaleX), Q_ARG(std::vector<QPixmap>*, &mMfccPix)); // mAnalyze->createPixmapMfcc(mScaleX, &mMfccPix);

		break;

	case STATUS_FINISH_LYRICS:
	{
		//mLyricsPix.clear();
		//float ratio = (float)ui.openGLWidgetLyrics->height() / ((float)ui.openGLWidget->height() / 4.0);	// 上に表示されてるものとの比率
		//QMetaObject::invokeMethod(mAnalyze, "createPixmapLyrics", Qt::QueuedConnection, Q_ARG(int, mScaleX), Q_ARG(std::vector<QPixmap>*, &mLyricsPix), Q_ARG(float, ratio)); // mAnalyze->createPixmapLyrics(mScaleX, &mLyricsPix,ratio);

		typedef std::vector<std::pair<float, QString>> pair_timing_lyrics;
		qRegisterMetaType<std::vector<std::pair<float, QString>>>("pair_timing_lyrics");
		QMetaObject::invokeMethod(mAnalyze, "getTimingLyricsList", Qt::DirectConnection, Q_RETURN_ARG(pair_timing_lyrics, mTimingLyricsList)); // mAnalyze->createPixmapLyrics(mScaleX, &mLyricsPix,ratio);
		
		ui.pushOK->setEnabled(true);

		break;
	}
	case STATUS_FINISH_WRITEUTAU:	// UTAUデータ作成完了
	{
		utau_note_list result;
		qRegisterMetaType<utau_note_list>("utau_note_list");
		QMetaObject::invokeMethod(mAnalyze, "getUtauData", Qt::DirectConnection, Q_RETURN_ARG(utau_note_list, result)); // result = mAnalyze->getUtauData();

		auto utaudata = Control::get_instance().getUtauData();
		for (auto data : result) {
			utaudata->AddSectionNote(data);
		}

		Control::get_instance().exportUtau();
		// qApp->exit();
		QApplication::closeAllWindows();
		return;
	}
		break;
	default:
		break;
	}


	// スケールチェンジ
	if (mScaleChangeTime == 0) {
		createPixmap(); wSizeChanged(); paintEvent(NULL);
		mScaleChangeTime = -1;
	}
	if (mScaleChangeTime > 0) { mScaleChangeTime--; return; }


	// ステータス文字列取得
	QString msg = "";
	QMetaObject::invokeMethod(mAnalyze, "getStatusMsg", Qt::DirectConnection, Q_RETURN_ARG(QString, msg));	// msg = mAnalyze->getStatusMsg();
	if (msg.length() > 0) msg = QString("ステータス:%1").arg(msg);
	if (ui.labelStatus->text() != msg)
		ui.labelStatus->setText(msg);
	int p[2];
	QMetaObject::invokeMethod(mAnalyze, "getStatusP", Qt::DirectConnection, Q_RETURN_ARG(int, p[0]), Q_ARG(int, 0));	// p[0] = mAnalyze->getStatusP(0);
	QMetaObject::invokeMethod(mAnalyze, "getStatusP", Qt::DirectConnection, Q_RETURN_ARG(int, p[1]), Q_ARG(int, 1));	// p[1] = mAnalyze->getStatusP(1);
	ui.progressBar->setMaximum(p[1]);
	ui.progressBar->setValue(p[0]);
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
	if (mMfccPix.size() > 0) lists.push_back(mMfccPix);
	lists.push_back(mPitchPix);

	for (auto list : lists) {
		tsize = 0;
		for (auto pix : list) {
			auto pix_ = pix.scaledToHeight(h / 3);
			// mfccが追加されたら
			if (mMfccPix.size() > 0) pix_ = pix.scaledToHeight(h / 4);
			auto rect1 = QRect(tsize, 0, pix_.width(), pix_.height());	// pixmapの領域
			auto rect2 = QRect(scrollX, 0, w, /*h / 3*/pix_.height());	// 表示領域
			

			if (rect1.intersects(rect2)) {	// 表示領域に入っていたら
				QPainter p(ui.openGLWidget);

				x = (rect1.x() > rect2.x()) ? rect1.x() - rect2.x() : 0;
				sx = (rect1.x() < rect2.x()) ? rect2.x() - rect1.x() : 0;
				sw = rect1.intersected(rect2).width(); // (rect2.x() < rect1.right()) ? rect1.right() - rect2.x() : (rect2.right() > rect1.x()) ? rect2.right() - rect1.x() : rect1.width();
				
				p.drawPixmap(x, pix_.height()*count, pix_, sx, 0, sw, pix_.height());
				
				p.end();
			}
			tsize += pix_.width();
		}
		count++;
	}


	// 歌詞
	if (mTimingLyricsList.size() > 0) {
		w = ui.openGLWidgetLyrics->width(), h = ui.openGLWidgetLyrics->height();
		//// ペンの準備
		painter.begin(ui.openGLWidgetLyrics);
		painter.setPen(QPen(Qt::white));
		painter.setBrush(QBrush(Qt::black));

		// キャンバスの初期化
		painter.eraseRect(0, 0, w, h);
		painter.drawRect(0, 0, w, h);
		painter.end();

		QFont font; font.setPixelSize(h);
		for (auto tl : mTimingLyricsList) {
			auto posX = tl.first * tsize;
			auto rect1 = QRect(posX, 0, h + 3, h);	// 歌詞の領域
			auto rect2 = QRect(scrollX, 0, w, h);	// 表示領域


			if (rect1.intersects(rect2)) {	// 表示領域に入っていたら
				QPainter p(ui.openGLWidgetLyrics);

				x = rect1.x() - rect2.x();
				p.setPen(QPen(QColor(255, 255, 255), 5));
				p.drawLine(x, 0, x, h);
				p.setFont(font);
				p.drawText(QPoint(x + 2, h), tl.second);

				p.end();
			}
		}
	}

}

void Analysis::analyze(QString filename) {
	if (mAnalyze) {
		mThread->quit();
		delete mAnalyze; 
	}
	mAnalyze = new Analyze(this);
	mThread = new QThread;
	mAnalyze->setParent(NULL);
	mAnalyze->moveToThread(mThread);
	mThread->start();

	qDebug() << QThread::currentThreadId();
	mFileName = filename;
	QMetaObject::invokeMethod(mAnalyze, "setMain", Qt::QueuedConnection, Q_ARG(KancopySaihu*, mParent));	// mAnalyze->setMain(mParent);
	QMetaObject::invokeMethod(mAnalyze, "init", Qt::QueuedConnection, Q_ARG(QString,filename));	// mAnalyze->init(filename);
	//mWaveData = mAnalyze->getData();
	//mWaveLength = mAnalyze->getSampleLength();
	//mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

	//createPixmap();	// 画像データ生成
	//wSizeChanged();	// スクロールバーの初期化
}

void Analysis::scaleUp() {
	if (mScaleX > 0) mScaleX -= 10; 
	if (mScaleX == 0) mScaleX = 1;
	mScaleChangeTime = 50;
	// createPixmap(); wSizeChanged(); paintEvent(NULL);
	ui.labelStatus->setText(QString("スケール:1/%1").arg(mScaleX));
}
void Analysis::scaleDown() {
	mScaleX += 10;
	mScaleChangeTime = 50;
	// createPixmap(); wSizeChanged(); paintEvent(NULL); 
	ui.labelStatus->setText(QString("スケール:1/%1").arg(mScaleX));
}
void Analysis::sliderChange() {  paintEvent(NULL); }
void Analysis::wSizeChanged() { 
	float ratio = (float)ui.horizontalScrollBar->value() / (float)ui.horizontalScrollBar->maximum();

	int ImageW = 0; // = mWavePix.scaledToHeight(ui.openGLWidget->height() / 2).width();
	for (auto pix : mWavePix) {
		if (mMfccPix.size() > 0) {
			ImageW += pix.scaledToHeight(ui.openGLWidget->height() / 4).width();
		}
		else {
			ImageW += pix.scaledToHeight(ui.openGLWidget->height() / 3).width();
		}
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

	if (mMfccPix.size() > 0) {
		mMfccPix.clear();
		QMetaObject::invokeMethod(mAnalyze, "createPixmapMfcc", Qt::QueuedConnection, Q_ARG(int, mScaleX), Q_ARG(std::vector<QPixmap>*, &mMfccPix)); // mAnalyze->createPixmapMfcc(mScaleX, &mMfccPix);
	}
	//if (mLyricsPix.size() > 0) {
	//	mLyricsPix.clear();
	//	float ratio = (float)ui.openGLWidgetLyrics->height() / ((float)ui.openGLWidget->height() / 4.0);	// 上に表示されてるものとの比率
	//	QMetaObject::invokeMethod(mAnalyze, "createPixmapLyrics", Qt::QueuedConnection, Q_ARG(int, mScaleX), Q_ARG(std::vector<QPixmap>*, &mLyricsPix), Q_ARG(float, ratio)); // mAnalyze->createPixmapMfcc(mScaleX, &mMfccPix);
	//}
}

void Analysis::reAnalyze() {
	analyze(mFileName);
}

void Analysis::confirm(){
	QMetaObject::invokeMethod(mAnalyze, "writeUtauData", Qt::QueuedConnection); // mAnalyze->writeUtauData();
}
