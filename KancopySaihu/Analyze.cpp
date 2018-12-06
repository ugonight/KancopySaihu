#pragma execution_character_set("utf-8")

#include "Analyze.h"
#include "wave.h"
#include "define.h"
#include "KancopySaihu.h"

#include "qpainter.h"
#include "qthread.h"
#include <qdebug.h>

// #include<fstream>


const int Analyze::fftsize = 1024;	// バッファサイズ
const float Analyze::dt = 0.001;		// 1ms秒


Analyze::Analyze(QObject *parent)
	: QObject(parent), mWaveRW(0), mStatus(STATUS_NONE), mStatusMsg("")
{
	// FFTW
	// const int fftsize = FRAMES_PER_BUFFER;
	mFFTW_In = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
	mFFTW_Out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
	mFFTW_Plan = fftw_plan_dft_1d(fftsize, mFFTW_In, mFFTW_Out, FFTW_FORWARD, FFTW_ESTIMATE);

	mFFTW_Result = 0;
	mFFTW_Pitch = 0;
}

Analyze::~Analyze()
{
	delete mWaveRW;
	fftw_free(mFFTW_In);
	fftw_free(mFFTW_Out);
	fftw_destroy_plan(mFFTW_Plan);
	if (mFFTW_Result) {
		int length = sizeof(mFFTW_Result) / fftsize;
		for (int i = 0; i < length; i++) {
			delete[] mFFTW_Result[i];
		}
		delete[] mFFTW_Result;
	}
	delete[] mFFTW_Pitch;
}

void Analyze::init(QString filename) {
	mStatusMsg = "初期化処理開始";
	mStatus = STATUS_PROCESS_INIT;

	qDebug() << QThread::currentThreadId();

	// wav読み込み
	mWaveRW = new waveRW();
	mWaveRW->wave_read(filename.toStdString().c_str());
	// qDebug("%d", mWaveRW->getSamplesPerSec());

	// std::ofstream ofs("fftw_pitch.txt");

	const int ms = mWaveRW->getLength() / (Fs * dt);	//1ms毎に取得

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
		mFFTW_Result = new float*[ms];	
		for (int i = 0; i < ms; i++) {
			*(mFFTW_Result + i) = new float[fftsize];
		}

		double window;
		for (int j = 0; j < ms; j++) {
			// 入力データ作成
			for (int i = 0; i < fftsize; i++) {
				window = (0.5 - 0.5 * cos(2 * pi * (((j*(Fs*dt)) + i) / fftsize)));	// 窓関数
				if ((int)(j*(Fs*dt)) + i < mWaveRW->getLength())
					mFFTW_In[i][0] = mWaveRW->getData()[(int)(j*(Fs*dt)) + i] * window;
				else
					mFFTW_In[i][0] = 0;
				mFFTW_In[i][1] = 0.0;
			}
			// FFTW実行
			fftw_execute(mFFTW_Plan);
			// 結果代入
			for (int i = 0; i < fftsize; i++) {
				mFFTW_Out[i][0] /= fftsize;	mFFTW_Out[i][1] /= fftsize;	// 正規化
				 mFFTW_Result[j][i] = sqrt(mFFTW_Out[i][0] * mFFTW_Out[i][0] + mFFTW_Out[i][1] * mFFTW_Out[i][1]);
				// if (j == 5000) ofs << "in:" << mFFTW_In[i][0] << "\t\tout:" << mFFTW_Result[j][i] << std::endl;
			}

			mStatusMsg = QString("FFT処理: %1 / %2").arg(j).arg(ms);
		}
	}


	// ピッチ解析
	{
		// メモリ解放
		if (mFFTW_Pitch) {
			delete[] mFFTW_Pitch;
		}

		// 結果格納領域の確保
		mFFTW_Pitch = new float[ms];


		auto *acf = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
		auto *acf_ = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
		fftw_plan planb = fftw_plan_dft_1d(fftsize, acf, acf_, FFTW_BACKWARD, FFTW_ESTIMATE);

		const double freqL = 32.703 * pow(2.0, (double)mMain->getRangeL() / 12.0);
		const double freqH = 32.703 * pow(2.0, (double)mMain->getRangeH() / 12.0);
		
		for (int j = 0; j < ms; j++) {
			// パワースペクトルから自己相関関数に変換
			for (int i = 0; i < fftsize; i++) {
				acf[i][0] = pow(mFFTW_Result[j][i], 2.0);
				acf[i][1] = 0;
			}
			fftw_execute(planb);	// 逆変換
			for (int i = 0; i < fftsize; i++) {
				acf_[i][0] *= 2.0 / fftsize;
				acf_[i][1] *= 2.0 / fftsize;
			}
			// ピークを探す
			double max_value = DBL_MIN;
			int max_idx = 0;
			double dy = 0, dy_, f;
			for (int i = 0; i < fftsize; ++i)
			{
				dy_ = dy;
				dy = acf_[i][0] - acf_[i - 1][0];
				f = (float)Fs / (float)i;
				if (dy_ > 0 && dy <= 0 && f >= freqL && f <= freqH)
				{
					if (acf_[i][0] > max_value)
					{
						max_value = acf_[i][0];
						max_idx = i;
					}
				}
			}
			double peakQuefrency = 1.0 / Fs * max_idx;
			double f0 = 1.0 / peakQuefrency;
			mFFTW_Pitch[j] = f0;
			// qDebug() << "max_value:" << max_value << "\t\tmax_idx:" << max_idx << "\t\tpitch:" << mFFTW_Pitch[j] /*<< std::endl*/;
			mStatusMsg = QString("ピッチ解析処理: %1 / %2").arg(j).arg(ms);
		}
		fftw_free(acf);
		fftw_free(acf_);
		fftw_destroy_plan(planb);
	}

	mStatus = STATUS_FINISH_INIT;
	mStatusMsg = "初期化処理完了";
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

AStatus Analyze::getStatus() {
	return mStatus;
}

QString Analyze::getStatusMsg() {
	return mStatusMsg;
}

void Analyze::createPixmap(int scale, std::vector<QPixmap> *wave, std::vector<QPixmap> *spect, std::vector<QPixmap> *pitch) {
	double length = mWaveRW->getLength(), w, h = 500.0;
	double *wavedata = mWaveRW->getData();
	int fftnum = mWaveRW->getLength() / (Fs * dt);
	int count = 0, tsize = 0;
	mStatus = STATUS_PROCESS_CREATEPIX;
	mStatusMsg = "描画処理開始";
	
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
		// double maxW = 0.0;	// 音声ファイルでの最大振幅
	
		for (int i = tsize; i < w + tsize; i++) {
			painter.drawLine(x, h / 2.0, x, h / 2.0 - wavedata[i] * (h / 2.0));
			x += 1.0 / scale;

			// if (wavedata[i] > maxW) maxW = wavedata[i];
			mStatusMsg = QString("wave波形描画: %1 / %2").arg(i).arg(w + tsize);
		}
		painter.end();
		wave->push_back(pix1);


		const double freqL = 32.703 * pow(2.0, (double)mMain->getRangeL() / 12.0);
		const double freqH = 32.703 * pow(2.0, (double)mMain->getRangeH() / 12.0);
		const double freqD = freqH - freqL;
		
		
		painter.begin(&pix2);
		painter.setPen(QPen(Qt::white));
		painter.setBrush(QBrush(Qt::black));
		painter.eraseRect(0, 0, w, h);
		painter.drawRect(0, 0, w, h);

		// スペクトログラム
		int y, y_ = 0, dt = (mWaveRW->getLength() / fftnum), value;
		
		x = 0.0;
		for (int i = tsize; i < w + tsize - dt; i += dt) {
			y_ = h;
			for (int j = 0; j < fftsize / 2; j++) {
				y = (double)h - (double)h * ((double)j / (double)(fftsize / 2.0));
				if (y == y_) continue; else y_ = y;

				value = mFFTW_Result[i / dt][j] * fftsize * 255.0;
				if (value < 0)value = 0.0;
				if (value > 255) value = 255;
				if ((Fs / fftsize) * j > freqL && (Fs / fftsize) * j < freqH)
					painter.setPen(QPen(QColor(value, value, value)));
				else
					painter.setPen(QPen(QColor(value, 0, 0)));

				painter.drawLine(x, y, x + (double)dt / (double)scale, y);
			}
			x += (double)dt / (double)scale;

			mStatusMsg = QString("スペクトログラム描画: %1 / %2").arg(i).arg(w + tsize - dt);
		}
		painter.end();
		spect->push_back(pix2);



		painter.begin(&pix3);
		painter.setPen(QPen(Qt::white));
		painter.setBrush(QBrush(Qt::black));
		painter.setFont(QFont("Arial", 20));
		painter.eraseRect(0, 0, w, h);
		painter.drawRect(0, 0, w, h);

		// ピッチ曲線


		x = 0.0, y_ = 0.0;
		double maxA, maxF, f;

		// 音階の境界表示
		const double df = pow(2.0, 1.0 / 12.0);
		double dy;
		for (int i = 0; i < mMain->getRangeH() - mMain->getRangeL(); i++) {
			if ((mMain->getRangeL() + i) % 12 == 0) 
				painter.setPen(QPen(Qt::blue, 3));	// Cは色を変える
			else
				painter.setPen(QPen(QColor(0, 0, 50), 3));
			
			dy = (h / (double)(mMain->getRangeH() - mMain->getRangeL()));
			y = h - i * dy;
			painter.drawLine(0, y, w, y);
		}
		// Cの文字を表示
		painter.setPen(QPen(Qt::white));
		for (int i = 0; i < mMain->getRangeH() - mMain->getRangeL(); i++)
			if ((mMain->getRangeL() + i) % 12 == 0)
				if (count == 0)  painter.drawText(0, h - i * (h / (double)(mMain->getRangeH() - mMain->getRangeL())), QString("C%1").arg((mMain->getRangeL() + i) / 12 + 1));


		for (int i = tsize; i < w + tsize-dt; i += dt) {
			// 指定された音域内で一番強い周波数を探す
			//maxA = 0.0, maxF = 0.0;
			//for (int j = 0; j < fftsize / 2; j++) {
			//	f = (Fs / fftsize) * j;
			//	if (f >= freqL && f <= freqH) {
			//		if (maxA < mFFTW_Result[i / dt][j]) {
			//			maxA = mFFTW_Result[i / dt][j];
			//			maxF = f;
			//		}
			//	}
			//}
			maxF = mFFTW_Pitch[i / dt];
			maxA = mFFTW_Result[i / dt][(int)(Fs / maxF)];
			
			y_ = y;
			y = h - ((log2(maxF) -log2(freqL)) / (log2(freqH) - log2(freqL))) * h;
			x += (double)dt / (double)scale;
			value = maxA * fftsize * 255.0;
			if (value > 255) value = 255;
			painter.setPen(QPen(QColor(value, value, value),3));

			painter.drawLine(x - (double)dt / (double)scale, y_, x, y);
			// ofs << "a:" << maxA << "\t\tf:" << maxF << "\t\tx:" << x << "\t\ty:" << y << std::endl;
			
			mStatusMsg = QString("ピッチ曲線描画: %1 / %2").arg(i).arg(w + tsize - dt);
		}

		painter.end();
		//pix2.toImage().save("spect.png");
		//pix3.toImage().save("pitch.png");

		pitch->push_back(pix3);

		tsize += w;
		count++;
	} while (w == maxSize * scale);

	mStatus = STATUS_FINISH_CREATEPIX;
	mStatusMsg = "描画処理完了";
}
