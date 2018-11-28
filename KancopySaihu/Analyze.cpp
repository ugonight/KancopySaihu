#pragma execution_character_set("utf-8")

#include "Analyze.h"
#include "wave.h"
#include "define.h"
#include "KancopySaihu.h"

#include "qpainter.h"

const int Analyze::fftsize = 1024;	// バッファサイズ
const float Analyze::dt = 0.001;		// 1ms秒

Analyze::Analyze(QObject *parent)
	: QObject(parent), mWaveRW(0)
{
	// FFTW
	// const int fftsize = FRAMES_PER_BUFFER;
	mFFTW_In = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
	mFFTW_Out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
	mFFTW_Plan = fftw_plan_dft_1d(fftsize, mFFTW_In, mFFTW_Out, FFTW_FORWARD, FFTW_ESTIMATE);
	mFFTW_Result = 0;
}

Analyze::~Analyze()
{
	delete mWaveRW;
	fftw_free(mFFTW_In);
	fftw_free(mFFTW_Out);
	fftw_destroy_plan(mFFTW_Plan);
}

void Analyze::init(QString filename) {
	// wav読み込み
	mWaveRW = new waveRW();
	mWaveRW->wave_read(filename.toStdString().c_str());
	// qDebug("%d", mWaveRW->getSamplesPerSec());


	// FFTW
	{	
		// メモリ解放
		if (mFFTW_Result) {
			int length = sizeof(mFFTW_Result) / fftsize;
			for (int i = 0; i < length; i++) {
				delete[] mFFTW_Result[i];
			}
			delete[] mFFTW_Result;
		}

		// 結果格納領域の確保
		const int ms = mWaveRW->getLength() / (Fs * dt);	//1ms毎に取得
		mFFTW_Result = new float*[ms];	
		for (int i = 0; i < ms; i++) {
			*(mFFTW_Result + i) = new float[fftsize];
		}

		for (int j = 0; j < ms; j++) {
			// 入力データ作成
			for (int i = 0; i < fftsize; i++) {
				if ((int)(j*(Fs*0.001)) + i < mWaveRW->getLength())
					mFFTW_In[i][0] = mWaveRW->getData()[(int)(j*(Fs*0.001)) + i];
				else
					mFFTW_In[i][0] = 0;
				mFFTW_In[i][1] = 0.0;
			}
			// FFTW実行
			fftw_execute(mFFTW_Plan);
			// 結果代入

			for (int i = 0; i < fftsize; i++) {
				mFFTW_Result[j][i] = sqrt(mFFTW_Out[i][0] * mFFTW_Out[i][0] + mFFTW_Out[i][1] * mFFTW_Out[i][1]) / sqrt(2.0);
			}
		}
	}
}

void Analyze::setMain(KancopySaihu *k) {
	mMain = k;
}


double* Analyze::getData() {
	return mWaveRW->getData();
}

long int Analyze::getSampleLength() {
	return mWaveRW->getLength();
}

float** Analyze::getFFTWResult(int *i, int *j) {
	if (i) *i = mWaveRW->getLength() / (Fs * dt);
	if (j) *j = fftsize;
	return mFFTW_Result;
}

void Analyze::createPixmap(int scale, std::vector<QPixmap> *wave, std::vector<QPixmap> *spect, std::vector<QPixmap> *pitch) {
	double length = mWaveRW->getLength(), w, h = 500.0;
	double *wavedata = mWaveRW->getData();
	int fftnum = mWaveRW->getLength() / (Fs * dt);
	int count = 0, tsize = 0;
	
	const int maxSize = 30000;	// Pixmapの上限値

	do
	{
		if (length / scale > maxSize) {
			w = maxSize * scale;
			length -= w;
		}
		else {
			w = length;
		}

		// Pixmapの作成
		QPixmap pix1(w / scale, h), pix2(w / scale, h), pix3(w / scale, h);

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
		for (int i = tsize; i < w + tsize; i++) {
			painter.drawLine(x, h / 2.0, x, h / 2.0 - wavedata[i] * (h / 2.0));
			x += 1.0 / scale;
		}
		painter.end();


		painter.begin(&pix2);
		painter.setPen(QPen(Qt::white));
		painter.setBrush(QBrush(Qt::black));
		painter.eraseRect(0, 0, w, h);
		painter.drawRect(0, 0, w, h);

		// スペクトログラム
		int y, y_ = 0, dt = (mWaveRW->getLength() / fftnum), value;
		x = 0.0;
		for (int i = tsize; i < w + tsize; i += dt) {
			y_ = h;
			for (int j = 0; j < fftsize / 2; j++) {
				y = (double)h - (double)h * ((double)j / (double)(fftsize / 2.0));
				if (y == y_) continue; else y_ = y;

				value = mFFTW_Result[i / dt][j] * 255.0;
				if (value > 255) value = 255;
				painter.setPen(QPen(QColor(value, value, value)));
				painter.drawLine(x, y, x + (double)dt / (double)scale, y);
			}
			x += (double)dt / (double)scale;
		}
		painter.end();



		painter.begin(&pix3);
		painter.setPen(QPen(Qt::white));
		painter.setBrush(QBrush(Qt::black));
		painter.eraseRect(0, 0, w, h);
		painter.drawRect(0, 0, w, h);

		// ピッチ曲線
		const double freqL = 32.703 * pow(2.0, (double)mMain->getRangeL() / 12.0);
		const double freqH = 32.703 * pow(2.0, (double)mMain->getRangeH() / 12.0);
		const double freqD = freqH - freqL;

		x = 0.0, y_ = 0.0;
		double maxA, maxF, f;
		for (int i = tsize; i < w + tsize; i += dt) {
			// 指定された音域内で一番強い周波数を探す
			maxA = 0.0, maxF = 0.0;
			for (int j = 0; j < fftsize / 2; j++) {
				f = (Fs / fftsize) * j;
				if (f >= freqL && f <= freqH) {
					if (maxA < mFFTW_Result[i / dt][j]) {
						maxA = mFFTW_Result[i / dt][j];
						maxF = f;
					}
				}
			}
			
			y_ = y;
			y = h - ((maxF - freqL) / freqD) * h;
			x += (double)dt / (double)scale;

			painter.drawLine(x - (double)dt / (double)scale, y_, x , y);
		}
		painter.end();



		wave->push_back(pix1);
		spect->push_back(pix2);
		pitch->push_back(pix3);

		tsize += w;
		count++;
	} while (w == maxSize * scale);
}
