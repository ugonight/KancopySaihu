#pragma execution_character_set("utf-8")

#include "Analyze.h"
#include "wave.h"
#include "define.h"
// #include "KancopySaihu.h"
// #include "JuliusT.h"

#include "qpainter.h"
#include "qthread.h"
#include <qdebug.h>

#include<fstream>
#include <set>


const int Analyze::fftsize = 1024;	// バッファサイズ
const float Analyze::dt = 0.01;		// 10ms秒


Analyze::Analyze(QObject *parent)
	: QObject(parent), mWaveRW(0), mStatus(STATUS_NONE), mStatusMsg(""),mJulius(0),
	mDivId(-1), mDivMax(0)
{
	// FFTW
	// const int fftsize = FRAMES_PER_BUFFER;
	mFFTW_In = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
	mFFTW_Out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
	mFFTW_Plan = fftw_plan_dft_1d(fftsize, mFFTW_In, mFFTW_Out, FFTW_FORWARD, FFTW_ESTIMATE);

	mFFTW_Result = 0;
	mFFTW_Pitch = 0;

	mStatusP[0] = mStatusP[1] = 0;

	mMfccResult = std::make_tuple(nullptr, 0, 0);

	// タイマー
	startTimer(1000);
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

	float** mf = std::get<0>(mMfccResult);
	for (int i = 0; i < std::get<1>(mMfccResult); i++) {
		if (mf[i]) { delete[] mf[i]; mf[i] = 0; }
	}
	delete[] mf;
}

void Analyze::init(QString filename) {
	mStatusMsg = "初期化処理開始";
	mStatus = STATUS_PROCESS_INIT;

	qDebug() << QThread::currentThreadId();

	// wav読み込み
	mWaveRW = new waveRW();
	mWaveRW->wave_read(filename.toStdString().c_str());
	// qDebug("%d", mWaveRW->getSamplesPerSec());

	// Julius起動
	{
		if (mJulius) delete mJulius;
		mJulius = new JuliusT();
		mJulius->setParent(NULL);
		mThread = new QThread();
		mJulius->moveToThread(mThread);
		std::ofstream ofs("list.txt");
		ofs << filename.toStdString();
		ofs.close();
		qRegisterMetaType<std::string>("std::string");
		QMetaObject::invokeMethod(mJulius, "init", Qt::QueuedConnection, Q_ARG(std::string, "list.txt"));
		QMetaObject::invokeMethod(mJulius, "startRecog", Qt::QueuedConnection);
		mThread->start();
	}

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

		mStatusMsg = tr("FFT処理:");
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
				mFFTW_Out[i][0] /= (fftsize / 2);	mFFTW_Out[i][1] /= (fftsize / 2);	// 正規化
				 mFFTW_Result[j][i] = sqrt(mFFTW_Out[i][0] * mFFTW_Out[i][0] + mFFTW_Out[i][1] * mFFTW_Out[i][1]);
				// if (j == 5000) ofs << "in:" << mFFTW_In[i][0] << "\t\tout:" << mFFTW_Result[j][i] << std::endl;
			}


			mStatusP[0] = j;
			mStatusP[1] = ms;
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

		mStatusMsg = tr("ピッチ解析処理: ");
		
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

			mStatusP[0] = j;
			mStatusP[1] = ms;
		}
		fftw_free(acf);
		fftw_free(acf_);
		fftw_destroy_plan(planb);
	}


	mStatus = STATUS_FINISH_INIT;
	mStatusMsg = "初期化処理完了";
}

void Analyze::timerEvent(QTimerEvent *event) {
	// MFCC取得
	mfcc_tuple mfcc;
	qRegisterMetaType<mfcc_tuple>("mfcc_tuple");
	QMetaObject::invokeMethod(mJulius, "getMfccResult", Qt::DirectConnection, Q_RETURN_ARG(mfcc_tuple, mfcc));	// mfcc = mJulius->getMfccResult();
	if (std::get<1>(mfcc) != 0 && std::get<0>(mMfccResult) == 0) {
		//std::ofstream ofs("mfcc.txt");
		//for (int i = 0; i < std::get<1>(mfcc); i++) {
		//	for (int j = 0; j < std::get<2>(mfcc); j++) {
		//		ofs << std::get<0>(mfcc)[i][j]  << ",";
		//	}
		//	ofs << std::endl;
		//}
		//ofs.close();

		// 前データの解放
		float** mf = std::get<0>(mMfccResult);
		for (int i = 0; i < std::get<1>(mMfccResult); i++) {
			if (mf[i]) { delete[] mf[i]; mf[i] = 0; }
		}
		delete[] mf;

		// コピーする
		//mfcc_tuple temp;
		//float** data = new float*[std::get<1>(mfcc)];
		//for (int i = 0; i < std::get<1>(mfcc); i++) {
		//	data[i] = new float[std::get<2>(mfcc)];
		//	for (int j = 0; j < std::get<2>(mfcc); j++) {
		//		data[i][j] = std::get<0>(mfcc)[i][j];
		//	}
		//}
		//std::get<0>(temp) = data;
		//std::get<1>(temp) = std::get<1>(mfcc);
		//std::get<2>(temp) = std::get<2>(mfcc);

		mMfccResult = mfcc;

		analyzeTiming();
		if (mMain->getDivMode())
			analyzeLyrics();

		mStatus = STATUS_FINISH_JULIUS_FIRST;
	}

	// 歌詞取得
	QString result = "";
	int divid = -1;
	QMetaObject::invokeMethod(mJulius, "getResult", Qt::DirectConnection, Q_RETURN_ARG(QString, result));	// result = mJulius->getResult();
	QMetaObject::invokeMethod(mJulius, "getDivId", Qt::DirectConnection, Q_RETURN_ARG(int, divid));	// divid = mJulius->getDivId();
	if (mDivId == -2 && mLyricsResult.size() == 0) {
		mDivId = divid;
		// 解析完了
		mStatus = STATUS_FINISH_LYRICS;
		mStatusMsg = "歌詞解析完了";
		mStatusP[0] = mStatusP[1] = 0;
		qRegisterMetaType<std::vector<QString>>("std::vector<QString>");
		QMetaObject::invokeMethod(mJulius, "getResultList", Qt::DirectConnection, Q_RETURN_ARG(std::vector<QString>, mLyricsResult));	// mLyricsResult = mJulius->getResultList();
		// 最初と最後を取り除く
		for (auto l : mLyricsResult) {
			l.replace(0, 1, "");
			l.chop(1);
		}
		// 用済みのdivフォルダを消す
		// QDir("div").removeRecursively();
	} else
	if (mDivId != divid && result != "") {
		//result.replace(0, 1, "");
		//result.chop(1);
		//qDebug() << result;
		//mLyricsResult.push_back(result);
		mDivId = divid;
		mStatusMsg = "歌詞解析";
		mStatusP[0] = divid; mStatusP[1] = mDivMax;
	}
	// 分割しない時
	if (!mMain->getDivMode() && mStatus == STATUS_FINISH_CREATEPIX_MFCC) {
		// 前後を削る
		result.replace(0, 1, "");
		result.chop(1);
		// 最初は空白
		mLyricsResult.push_back("");
		
		std::set<QString> s{ "ぁ","ぃ","ぅ","ぇ","ぉ","ゃ","ゅ","ょ" };
		QString r;
		for (int i = 0; i < result.length(); i++) {
			r = result;
			r.replace(0, i, "");
			r.chop(result.length() - i - 1);
			if (s.count(r)) {
				//小文字（？）が含まれていたらリストの前のデータに加える
				mLyricsResult.at(mLyricsResult.size() - 1) += r;
			}
			else {
				mLyricsResult.push_back(r);
			}
		}
		// 解析完了
		mStatus = STATUS_FINISH_LYRICS;
		mStatusMsg = "歌詞解析完了";
		mStatusP[0] = mStatusP[1] = 0;

		for (auto l : mLyricsResult) {
			qDebug() << l;
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

AStatus Analyze::getStatus() {
	return mStatus;
}

QString Analyze::getStatusMsg() {
	return mStatusMsg;
}

int Analyze::getStatusP(int id) {
	return mStatusP[id];
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

		mStatusMsg = tr("wave波形描画: ");
		for (int i = tsize; i < w + tsize; i++) {
			painter.drawLine(x, h / 2.0, x, h / 2.0 - wavedata[i] * (h / 2.0));
			x += 1.0 / scale;

			// if (wavedata[i] > maxW) maxW = wavedata[i];
			mStatusP[0] = i;
			mStatusP[1] = (int)w+tsize;
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
		mStatusMsg = tr("スペクトログラム描画: ");
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


			mStatusP[0] = i;
			mStatusP[1] = (int)w + tsize - dt;
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

		mStatusMsg = tr("ピッチ曲線描画: ");

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
			
			mStatusP[0] = i;
			mStatusP[1] = (int)w + tsize - dt;
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
	mStatusP[0] = mStatusP[1] = 0;
}

void Analyze::createPixmapMfcc(int scale, std::vector<QPixmap> *mfcc) {
	double length = mWaveRW->getLength(), w, h = 500.0;
	double *wavedata = mWaveRW->getData();
	int count = 0, tsize = 0;

	mStatus = STATUS_PROCESS_CREATEPIX_MFCC;

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
		QPixmap pix1(w / scale, h);

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

		// MFCC
		float** mfccr = std::get<0>(mMfccResult);
		int samplenum = std::get<1>(mMfccResult);
		int mfccnum = std::get<2>(mMfccResult);
		int y, y_ = 0, d = (mWaveRW->getLength() / samplenum), value;

		x = 0.0;
		mStatusMsg = tr("MFCC描画: ");
		for (int i = tsize; i < w + tsize - d; i += d) {
			y_ = h;
			for (int j = 0; j < mfccnum; j++) {
				y = (double)h - (double)h * ((double)j / (double)(mfccnum));
				if (y == y_) continue; else y_ = y;

				value = (mfccr[(int)(i / d)][j] + 5.0) / 10.0  * 360.0;
				if (value < 0)value = 0.0;
				if (value > 360) value = 360;
				painter.setPen(QPen(QColor::fromHsv(value, 255, 255, 50)));
				painter.drawLine(x, y, x + (double)d / (double)scale, y);

			}
			x += (double)d / (double)scale;

			mStatusP[0] = i;
			mStatusP[1] = (int)w + tsize - d;
		}

		// 12次元成分のグラフ
		y = x = 0.0;
		painter.setPen(QPen(QColor(255, 255, 255), 3));
		mStatusMsg = tr("MFCC描画(12次元): ");
		for (int i = tsize; i < w + tsize - d; i += d) {
			y_ = y;
			y = (mfccr[(int)(i / d)][12] + 5.0) / 10.0  * h;
			x += (double)d / (double)scale;

			painter.drawLine(x - (double)d / (double)scale, y_, x, y);

			mStatusP[0] = i;
			mStatusP[1] = (int)w + tsize - d;
		}

		// 二乗和のグラフ
		y = x = 0.0;
		painter.setPen(QPen(QColor(255, 200, 200), 3));
		mStatusMsg = tr("MFCC描画(二乗和): ");
		// 二乗和の計算
		float *sum2= new float[samplenum];	
		float sum2max = 0;
		for (int i = 0; i < samplenum; i++) {
			sum2[i] = 0;
			for (int j = 40; j < 120; j++)
				sum2[i] += pow(mfccr[i][j], 2.0);
			if (sum2[i] > sum2max) sum2max = sum2[i];
		}

		for (int i = tsize; i < w + tsize - d; i += d) {
			y_ = y;
			y = sum2[(int)(i / d)] / sum2max * h;
			x += (double)d / (double)scale;

			painter.drawLine(x - (double)d / (double)scale, y_, x, y);

			mStatusP[0] = i;
			mStatusP[1] = (int)w + tsize - d;
		}

		delete[] sum2;


		// クオンタイズ・タイミングの位置
		y = x = 0.0;
		int id = 0;
		const float dm = ((float)mWaveRW->getLength() / (float)samplenum) / (float)Fs;	//	mfccの間隔[s]
		const float ds = 60.0 / (float)mMain->getTempo();
		const float dq = (4.0 * ds) / (float)mMain->getQuantize();	// クオンタイズの間隔[s]
		const int dd = (dq/dm) * ((float)mWaveRW->getLength() / (float)samplenum);	// 解析間隔
		const int offset = mMain->getOffset()*(float)Fs;
		for (int i = tsize; i < w + tsize; i++) {
			x += 1.0 / (double)scale;

			// クオンタイズ
			if ((i - offset) % (int)(dq*Fs)/*dd*/ == 0) {
				painter.setPen(QPen(QColor(100, 100, 255, 100), 5));
				painter.drawLine(x, 0, x, h);
			}
			// タイミング
			if (i == (int)mTimingResult[id]) {
				painter.setPen(QPen(QColor(255, 255, 255), 5));
				painter.drawLine(x, 0, x, h);
				if (id < mTimingResult.size()-1) id++;
			}
		}


		painter.end();
		mfcc->push_back(pix1);

		// pix1.save("mfcc.png");

		tsize += w;
		count++;
	} while (w == maxSize * scale);

	mStatus = STATUS_FINISH_CREATEPIX_MFCC;
	mStatusMsg = "描画処理完了";
	mStatusP[0] = mStatusP[1] = 0;
}

//void Analyze::createPixmapLyrics(int scale, std::vector<QPixmap> *mfcc, float ratio) {
//	double length = mWaveRW->getLength(), w, h = 25.0;
//	double *wavedata = mWaveRW->getData();
//	int count = 0, tsize = 0;
//
//	mStatus = STATUS_PROCESS_CREATEPIX_LYRICS;
//
//	const int maxSize = 30000;	// Pixmapの上限値
//	do
//	{
//		if (length / scale * ratio> maxSize) {
//			w = maxSize * scale * (1.0 / ratio);
//			length -= w;
//		}
//		else {
//			w = length;
//		}
//
//		// Pixmapの作成
//		QPixmap pix1(w / scale * ratio, h);
//
//		// ペンの準備
//		QPainter painter;
//		painter.begin(&pix1);
//		painter.setPen(QPen(Qt::white));
//		painter.setBrush(QBrush(Qt::black));
//
//		// キャンバスの初期化
//		painter.eraseRect(0, 0, w, h);
//		painter.drawRect(0, 0, w, h);
//
//		// wave波形
//		double x = 0.0;
//
//		// MFCC
//		float** mfccr = std::get<0>(mMfccResult);
//		int samplenum = std::get<1>(mMfccResult);
//		int mfccnum = std::get<2>(mMfccResult);
//		int y, y_ = 0, d = (mWaveRW->getLength() / samplenum), value;
//
//		// タイミングの位置
//		y = x = 0.0;
//		int id = 0;
//		const float dm = ((float)mWaveRW->getLength() / (float)samplenum) / (float)Fs;	//	mfccの間隔[s]
//		const float ds = 60.0 / (float)mMain->getTempo();
//		const float dq = (4.0 * ds) / (float)mMain->getQuantize();	// クオンタイズの間隔[s]
//		const int dd = (dq / dm) * ((float)mWaveRW->getLength() / (float)samplenum);	// 解析間隔
//		const int offset = mMain->getOffset()*(float)Fs;
//		QFont font; font.setPixelSize(h); painter.setFont(font);
//		for (int i = tsize; i < w + tsize; i++) {
//			x += 1.0 / (double)scale * ratio;
//
//			// タイミング
//			if (i == (int)mTimingResult[id]) {
//				painter.setPen(QPen(QColor(255, 255, 255), 5));
//				painter.drawLine(x, 0, x, h);
//
//				// 歌詞
//				if (id + 1 < mLyricsResult.size())
//					painter.drawText(QPoint(x + 2, h), mLyricsResult.at(id + 1));
//
//				if (id < mTimingResult.size() - 1) id++;
//			}
//		}
//
//
//		painter.end();
//		mfcc->push_back(pix1);
//
//		pix1.save("lyrics.png");
//
//		tsize += w;
//		count++;
//	} while (w == maxSize * scale);
//
//	mStatus = STATUS_FINISH_CREATEPIX_LYRICS;
//	mStatusMsg = "描画処理完了";
//	mStatusP[0] = mStatusP[1] = 0;
//}

std::vector<std::pair<float, QString>> Analyze::getTimingLyricsList() {
	std::vector<std::pair<float, QString>> result;
	double length = mWaveRW->getLength();
	float timing;

	result.push_back(std::make_pair(0.0, mLyricsResult[0]));

	for (int i = 0; i < mTimingResult.size(); i++) {
		timing = mTimingResult[i] / length;	// 全体の比で返す
		result.push_back(std::make_pair(timing, mLyricsResult[i + 1]));
	}

	mStatus = STATUS_GET_LYRICS;
	return result;
}

void Analyze::analyzeTiming() {
	auto mfcc = std::get<0>(mMfccResult);
	auto samplenum = std::get<1>(mMfccResult);
	auto vecnum = std::get<2>(mMfccResult);
	mTimingResult.clear();

	// std::ofstream ofs("timing.txt");

	// 二乗和の計算
	float *sum2 = new float[samplenum];
	float sum2max = 0;
	for (int i = 0; i < samplenum; i++) {
		sum2[i] = 0;
		for (int j = 40; j < 120; j++)
			sum2[i] += pow(mfcc[i][j], 2.0);
		if (sum2[i] > sum2max) sum2max = sum2[i];
	}

	const float threshold = sum2max * mMain->getThreshold() / 100.0; // 最大値の~%を閾値とする
	const float dd = ((float)mWaveRW->getLength() / (float)samplenum);
	const float dm = dd / (float)Fs;	//	mfccの間隔[s]
	const float ds = 60.0 / (float)mMain->getTempo();
	const float dq = (4.0 * ds) / (float)mMain->getQuantize();	// クオンタイズの間隔[s]
	const float d = dq / dm;	// 解析間隔(配列)

	//for (int i = d; i < samplenum; i += d) {
	//	if (abs(sum2[i - (int)1] - sum2[i]) > threshold) {
	//		mTimingResult.push_back(i*dd);	// 音声データのサンプリング単位でタイミングを出す
	//		ofs << i * dd << std::endl;
	//	}
	//}
	std::vector<float> cand;
	for (int i = 0; i < samplenum; i++) {
		if (abs(sum2[i - 1] - sum2[i]) > threshold) {
			cand.push_back(i*dd);	// 音声データのサンプリング単位でタイミングを出す
			// ofs << i * dd << std::endl;
		}
	}
	// クオンタイズにあったものを採用
	int offset = mMain->getOffset()*(float)Fs / dd;
	for (int i = offset; i < samplenum; i += d) {
		for (auto c : cand) {
			if (abs(c/dd - i) < 3.0) {
				mTimingResult.push_back(c);
				break;
			}
		}
	}

	// ofs.close();

	delete[] sum2;
}

void Analyze::analyzeLyrics() {
	auto wavrw = new waveRW();
	auto src = mWaveRW->getData();
	double* dst;
	std::vector<double> data;
	float ti_ = 0.0;
	int size, overlap, blank, i, id = 0;
	if (!QDir("").exists("div")) QDir("").mkdir("div");	// 保存ディレクトリの作成
	std::ofstream ofs("div\\list.txt");
	overlap = (Fs / 1000) * 100;	// オーバーラップ:100ms
	blank = (Fs / 1000) * 200;	// ブランク:200ms

	// 分割wavの生成
	for (auto ti : mTimingResult) {
		if (ti_ < overlap) overlap = ti_;

		// 前ブランク
		for (int i = 0; i < blank; i++) data.push_back(0.0);

		//size = (int)(ti - ti_) + overlap + 1;
		//dst = new double[size];
		// オーバーラップ
		for (i = 0; i < overlap; i++) {
			// dst[i] = src[(int)ti_ + i - overlap];
			data.push_back(src[(int)ti_ + i - overlap]);
		}
		
		// コピー
		for (i = 0; i < ti - ti_; i++) {
			// dst[i + overlap] = src[(int)ti_ + i];
			data.push_back(src[(int)ti_ + i]);
		}

		// 後ブランク
		for (int i = 0; i < blank; i++) data.push_back(0.0);

		// 配列にする
		size = data.size();
		dst = new double[size];
		for (int i = 0; i < size; i++)dst[i] = data[i];

		// 保存
		wavrw->setSamplesPerSec(Fs);
		wavrw->setDataChunkSize(size * 2);
		wavrw->setLength(size);
		wavrw->setBitsPerSample(16);
		wavrw->setBlockSize(16 / 8);
		wavrw->setBytesPerSec(wavrw->getBlockSize() * Fs);
		wavrw->setRiffChunkSize(36 + wavrw->getDataChunkSize());
		wavrw->setData(dst);
		auto path = QString::asprintf("div\\%d.wav", id).toStdString();
		wavrw->wave_write(path.c_str());
		ofs << QDir::currentPath().toStdString() << "\\" << path << std::endl;

		ti_ = ti;
		id++;
		delete[] dst; 
		data.clear();
	}

	delete wavrw;


	// Juliusに渡す
	qRegisterMetaType<std::string>("std::string");
	QMetaObject::invokeMethod(mJulius, "init", Qt::QueuedConnection, Q_ARG(std::string, "div\\list.txt"));
	QMetaObject::invokeMethod(mJulius, "startRecog", Qt::QueuedConnection);
	QMetaObject::invokeMethod(mJulius, "setDivMode", Qt::DirectConnection, Q_ARG(bool, true));
	QMetaObject::invokeMethod(mJulius, "setDivMax", Qt::DirectConnection, Q_ARG(int, id+1));

	mDivMax = id+1;
}