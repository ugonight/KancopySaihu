#pragma execution_character_set("utf-8")

#include "analysis.h"
#include "Analyze.h"
#include"KancopySaihu.h"

#include<qpainter.h>
#include<qtimer.h>

Analysis::Analysis(QWidget *parent)
	: QWidget(parent), mAnalyze(0),mWaveData(0),mScaleX(10), mFFTWData(0)
{
	ui.setupUi(this);

	mAnalyze = new Analyze(this);

	// タイマー
	auto timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(1000 /*/ 30*/);	// 30fps
}

Analysis::~Analysis()
{
	delete mAnalyze;
}

void Analysis::setMain(KancopySaihu *parent) {
	mParent = parent;
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
	mAnalyze->init(filename);
	mAnalyze->setMain(mParent);
	mWaveData = mAnalyze->getData();
	mWaveLength = mAnalyze->getSampleLength();
	mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

	createPixmap();	// 画像データ生成
	wSizeChanged();	// スクロールバーの初期化
}

void Analysis::scaleUp() { if (mScaleX > 0) mScaleX -= 10;  createPixmap(); wSizeChanged(); paintEvent(NULL); }
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

	mAnalyze->createPixmap(mScaleX, &mWavePix, &mSpectPix, &mPitchPix);

	//double w = mWaveLength, h = 500.0;

	//// Pixmapの作成
	//QPixmap pix1(w / mScaleX, h), pix2(w / mScaleX, h);

	//// ペンの準備
	//QPainter painter;
	//painter.begin(&pix1);
	//painter.setPen(QPen(Qt::white));
	//painter.setBrush(QBrush(Qt::black));

	//// キャンバスの初期化
	//painter.eraseRect(0, 0, w, h);
	//painter.drawRect(0, 0, w, h);

	//// wave波形
	//double x = 0.0;
	//for (int i = 0; i < w; i++) {
	//	painter.drawLine(x, h / 2.0, x, h / 2.0 - mWaveData[i] * (h / 2.0));
	//	x += 1.0 / mScaleX;
	//}
	//painter.end();
	//

	//painter.begin(&pix2);
	//painter.setPen(QPen(Qt::white));
	//painter.setBrush(QBrush(Qt::black));
	//painter.eraseRect(0, 0, w, h);
	//painter.drawRect(0, 0, w, h);

	//// スペクトログラム
	//int y, dt = (mWaveLength / mFFTnum);
	//x = 0.0;
	//for (int i = 0; i < w; i += dt) {
	//	for (int j = 0; j < mFFTsize / 2; j++) {
	//		y = (double)h - (double)h * ((double)j / (double)(mFFTsize / 2.0));

	//		int value = mFFTWData[i / dt][j] * 255.0;
	//		if (value > 255) value = 255;
	//		painter.setPen(QPen(QColor(value, value, value)));
	//		painter.drawPoint(x, y);
	//	}
	//	x+= dt / mScaleX;
	//}
	//painter.end();


	//mWavePix = pix1;
	//mSpectPix = pix2;

	// 画像出力

	//const QImage img = pix1.toImage();//QGraphicsPixmapItemの関数でQPixmapにアクセスして、QPixmapを QImageに変換
	//const QImage img2 = pix2.toImage();//QGraphicsPixmapItemの関数でQPixmapにアクセスして、QPixmapを QImageに変換
	//QImage image;//テンポラリ

	//if (img.format() != QImage::Format_RGB32) {//RGB32フォーマットかを確認した上で必要ならデータ変換する。
	//	image = img.convertToFormat(QImage::Format_RGB32);
	//}
	//else
	//	image = img;
	//image.save("wave.png", 0, -1);

	//if (img2.format() != QImage::Format_RGB32) {//RGB32フォーマットかを確認した上で必要ならデータ変換する。
	//	image = img2.convertToFormat(QImage::Format_RGB32);
	//}
	//else
	//	image = img2;
	//image.save("spect.png", 0, -1);
}