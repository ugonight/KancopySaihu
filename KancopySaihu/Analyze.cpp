#include "Analyze.h"
#include "wave.h"
#include "define.h"

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
	qDebug("%d", mWaveRW->getSamplesPerSec());


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