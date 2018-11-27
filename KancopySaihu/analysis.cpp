#include "analysis.h"
#include "Analyze.h"

#include<QtGui>
#include<fstream>

Analysis::Analysis(QWidget *parent)
	: QWidget(parent), mAnalyze(0),mWaveData(0),mScaleX(50), mFFTWData(0)
{
	ui.setupUi(this);

	mAnalyze = new Analyze(this);

	// �^�C�}�[
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

	//// �y���̏���
	QPainter painter(ui.openGLWidget);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));

	// �L�����o�X�̏�����
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);


	//// ��i:wave�g�`
	//double x = 0.0;
	//for (int i = 0; x < w && i + ui.horizontalScrollBar->value() * (int)mScaleX < mWaveLength; i++) {
	//	painter.drawLine(x, h / 4, x, h / 4 - mWaveData[i + ui.horizontalScrollBar->value() * (int)mScaleX] * (h / 4));
	//	x += 1.0 / mScaleX;
	//}


	//// ���i:�X�y�N�g���O����
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

	wSizeChanged();	// �X�N���[���o�[�̏�����
	createPixmap();	// �摜�f�[�^����
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
}	// �X�N���[���o�[�̏�����


void Analysis::createPixmap() {
	double w = mWaveLength, h = 500.0;

	// Pixmap�̍쐬
	QPixmap pix1(w / mScaleX, h), pix2(w / mScaleX, h);

	// �y���̏���
	QPainter painter;
	painter.begin(&pix1);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));

	// �L�����o�X�̏�����
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);

	// wave�g�`
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

	// �X�y�N�g���O����
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

	// �摜�o��

	//const QImage img = pix1.toImage();//QGraphicsPixmapItem�̊֐���QPixmap�ɃA�N�Z�X���āAQPixmap�� QImage�ɕϊ�
	//const QImage img2 = pix2.toImage();//QGraphicsPixmapItem�̊֐���QPixmap�ɃA�N�Z�X���āAQPixmap�� QImage�ɕϊ�
	//QImage image;//�e���|����

	//if (img.format() != QImage::Format_RGB32) {//RGB32�t�H�[�}�b�g�����m�F������ŕK�v�Ȃ�f�[�^�ϊ�����B
	//	image = img.convertToFormat(QImage::Format_RGB32);
	//}
	//else
	//	image = img;
	//image.save("wave.png", 0, -1);

	//if (img2.format() != QImage::Format_RGB32) {//RGB32�t�H�[�}�b�g�����m�F������ŕK�v�Ȃ�f�[�^�ϊ�����B
	//	image = img2.convertToFormat(QImage::Format_RGB32);
	//}
	//else
	//	image = img2;
	//image.save("spect.png", 0, -1);
}