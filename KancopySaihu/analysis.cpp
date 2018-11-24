#include "analysis.h"
#include "Analyze.h"

#include<QtGui>
#include<fstream>

Analysis::Analysis(QWidget *parent)
	: QWidget(parent), mAnalyze(0),mWaveData(0),mScaleX(100), mFFTWData(0)
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

	// �y���̏���
	QPainter painter(ui.openGLWidget);
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));

	// �L�����o�X�̏�����
	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);


	// ��i:wave�g�`
	double x = 0.0;
	for (int i = 0; x < w && i + ui.horizontalScrollBar->value() * (int)mScaleX < mWaveLength; i++) {
		painter.drawLine(x, h / 4, x, h / 4 - mWaveData[i + ui.horizontalScrollBar->value() * (int)mScaleX] * (h / 4));
		x += 1.0 / mScaleX;
	}
	// ���i:�X�y�N�g���O����
	 x = 0.0;
	 int y, y_ = -1,pos = -1, dt = (mWaveLength / mFFTnum);

	for (int i = 0; x < w && pos < mWaveLength; i+= dt) {
		pos = (i + ui.horizontalScrollBar->value() * (int)mScaleX);
			for (int j = 0; j < mFFTsize / 2; j++) {
				y = (double)h - ((double)h / 2.0) * ((double)j / (double)(mFFTsize / 2.0));
				if (y != y_) {
					int value = mFFTWData[pos / dt][j] * 255.0;
					if (value > 255) value = 255;
					painter.setPen(QPen(QColor(value, value, value)));
					painter.drawPoint(x, y);
					y_ = y;
				}
			}
		x += dt / mScaleX ;
	}
}

void Analysis::analyze(QString filename) {
	mAnalyze->init(filename);
	mWaveData = mAnalyze->getData();
	mWaveLength = mAnalyze->getSampleLength();
	mFFTWData = mAnalyze->getFFTWResult(&mFFTnum, &mFFTsize);

	wSizeChanged();	// �X�N���[���o�[�̏�����
}

void Analysis::scaleUp() { if (mScaleX > 0) mScaleX -= 10; wSizeChanged(); paintEvent(NULL); }
void Analysis::scaleDown() { mScaleX += 10; wSizeChanged(); paintEvent(NULL); }
void Analysis::sliderChange() {  paintEvent(NULL); }
void Analysis::wSizeChanged() { 
	ui.horizontalScrollBar->setMaximum(mWaveLength / mScaleX - ui.openGLWidget->width()); 
	qDebug("%d", ui.horizontalScrollBar->value());
}	// �X�N���[���o�[�̏�����
