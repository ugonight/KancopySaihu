#include "analysis.h"
#include "Analyze.h"

#include<QtGui>
#include<fstream>

Analysis::Analysis(QWidget *parent)
	: QWidget(parent), mAnalyze(0),mWaveData(0),mScaleX(50), mFFTWData(0)
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

void Analysis::paintEvent(QPaintEvent *) {
	
	int w = ui.openGLWidget->width(), h = ui.openGLWidget->height();

	//// ペンの準備
	QPainter painter(ui.openGLWidget);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));

	// キャンバスの初期化
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);


	//// 上段:wave波形
	//double x = 0.0;
	//for (int i = 0; x < w && i + ui.horizontalScrollBar->value() * (int)mScaleX < mWaveLength; i++) {
	//	painter.drawLine(x, h / 4, x, h / 4 - mWaveData[i + ui.horizontalScrollBar->value() * (int)mScaleX] * (h / 4));
	//	x += 1.0 / mScaleX;
	//}


	//// 下段:スペクトログラム
	// x = 0.0;
	// int y, y_ = -1,pos = -1, dt = (mWaveLength / mFFTnum);

	//for (int i = 0; x < w && pos < mWaveLength; i+= dt) {
	//	pos = (i + ui.horizontalScrollBar->value() * (int)mScaleX);
	//		for (int j = 0; j < mFFTsize / 2; j++) {
	//			y = (double)h - ((double)h / 2) * ((double)j / (double)(mFFTsize / 2.0));
	//			if (y != y_) {
	//				int value = mFFTWData[pos / dt][j] * 255.0;
	//				if (value > 255) value = 255;
	//				painter.setPen(QPen(QColor(value, value, value)));
	//				painter.drawPoint(x, y);
	//				y_ = y;
	//			}
	//		}
	//	x += dt / mScaleX ;
	//}

	int scrollX = ui.horizontalScrollBar->value();
	QPixmap pix = mWavePix.scaledToHeight(h / 2);
	painter.drawPixmap(0, 0, pix, scrollX, 0, w, h / 2);
	painter.end();
	painter.begin(ui.openGLWidget);
	pix = mSpectPix.scaledToHeight(h / 2);
	painter.drawPixmap(0, h / 2, pix, scrollX, 0, w, h / 2);
	painter.end();
}

void Analysis::analyze(QString filename) {
	mAnalyze->init(filename);
	mWaveData = mAnalyze->getData();
	mWaveLength = mAnalyze->getSampleLength();
	mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

	wSizeChanged();	// スクロールバーの初期化
	createPixmap();	// 画像データ生成
}

void Analysis::scaleUp() { if (mScaleX > 0) mScaleX -= 10;  createPixmap(); wSizeChanged(); paintEvent(NULL); }
void Analysis::scaleDown() { mScaleX += 10; createPixmap(); wSizeChanged(); paintEvent(NULL); }
void Analysis::sliderChange() {  paintEvent(NULL); }
void Analysis::wSizeChanged() { 
	int ImageW = mWavePix.scaledToHeight(ui.openGLWidget->height() / 2).width();
	if (ImageW > ui.openGLWidget->width()) {
		ui.horizontalScrollBar->setMaximum(ImageW - ui.openGLWidget->width());
	}
	else {
		ui.horizontalScrollBar->setMaximum(0);
	}
	qDebug("%d", ui.horizontalScrollBar->value());
}	// スクロールバーの初期化


void Analysis::createPixmap() {
	double w = mWaveLength, h = 500.0;

	// Pixmapの作成
	QPixmap pix1(w / mScaleX, h), pix2(w / mScaleX, h);

	// ペンの準備
	QPainter painter;
	painter.begin(&pix1);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));

	// キャンバスの初期化
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);

	// wave波形
	double x = 0.0;
	for (int i = 0; i < w; i++) {
		painter.drawLine(x, h / 2.0, x, h / 2.0 - mWaveData[i] * (h / 2.0));
		x += 1.0 / mScaleX;
	}
	painter.end();
	

	painter.begin(&pix2);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);

	// スペクトログラム
	int y, dt = (mWaveLength / mFFTnum);
	x = 0.0;
	for (int i = 0; i < w; i += dt) {
		for (int j = 0; j < mFFTsize / 2; j++) {
			y = (double)h - (double)h * ((double)j / (double)(mFFTsize / 2.0));

			int value = mFFTWData[i / dt][j] * 255.0;
			if (value > 255) value = 255;
			painter.setPen(QPen(QColor(value, value, value)));
			painter.drawPoint(x, y);
		}
		x+= dt / mScaleX;
	}
	painter.end();


	mWavePix = pix1;
	mSpectPix = pix2;

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