#pragma execution_character_set("utf-8")

#include "record.h"
#include "KancopySaihu.h"
#include "define.h"
#include "wave.h"
#include "JuliusT.h"

#include <QDir>
#include <QtWidgets>

std::vector<double> Record::mRecData;
std::vector<Record::dWave> Record::mWaveData;
bool Record::mRecNow = false;

Record::Record(QWidget *parent)
	: QWidget(parent), mDispMode(0)
{
	ui.setupUi(this);

	// デフォルトの録音の保存先を指定
	QDir curDir(QDir::current());
	QString canonicalDir(curDir.canonicalPath());
	ui.lineFileName->setText(canonicalDir + "\\record.wav");


	// 初期化

	// PortAudio
	{
		PaError err;
		PaStreamParameters inParam, outParam; //入出力の定義
		Pa_Initialize();	//PortAudio初期化
		inParam.device = Pa_GetDefaultInputDevice(); /* デフォルトインプットデバイス */
		if (inParam.device == paNoDevice) {
			printf("Error: No default input device.\n");
			Pa_Terminate();
			return;
		}
		inParam.channelCount = 1; /* モノラルインプット */
		inParam.sampleFormat = paFloat32;
		inParam.suggestedLatency = Pa_GetDeviceInfo(inParam.device)->defaultLowInputLatency;
		inParam.hostApiSpecificStreamInfo = NULL;
		//出力の設定
		outParam.device = Pa_GetDefaultOutputDevice(); //デフォルトのオーディオデバイス
		outParam.channelCount = 1;
		outParam.sampleFormat = paFloat32; //32bit floatで処理
		outParam.suggestedLatency = Pa_GetDeviceInfo(outParam.device)->defaultLowOutputLatency;
		outParam.hostApiSpecificStreamInfo = NULL;

		// FFTW
		const int fftsize = FRAMES_PER_BUFFER;
		mData = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
		mOut = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * fftsize);
		mPlan = fftw_plan_dft_1d(fftsize, mData, mOut, FFTW_FORWARD, FFTW_ESTIMATE);

		//PortAudioオープン
		Pa_OpenStream(
			&mStream,
			&inParam,
			&outParam,
			Fs,
			FRAMES_PER_BUFFER,
			paClipOff,
			Record::dsp,
			mData);
		//PortAudioスタート
		Pa_StartStream(mStream);
	}

	// タイマー
	auto timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(1000 / 30);	// 30fps

	// ステータスバー
	// mStatusBar = new QStatusBar(this);

	// Julius
	QMessageBox msgBox(this);
	msgBox.setWindowTitle("juliusを起動しています…");
	msgBox.setStandardButtons(NULL);
	msgBox.show();
	mJuliusT = new JuliusT(this);
	auto thread = new QThread();
	mJuliusT->setParent(NULL);
	mJuliusT->moveToThread(thread);	
	thread->start();
	mJuliusT->init();
	QMetaObject::invokeMethod(mJuliusT, "startRecog"); // mJuliusT->startRecog();
	msgBox.close();

	// 認識リスト
	mWordData.push_back("");
}

Record::~Record()
{
	//PortAudio終了
	Pa_StopStream(mStream);
	Pa_CloseStream(mStream);
	Pa_Terminate();
	// FFTW終了
	fftw_destroy_plan(mPlan);
	fftw_free(mData);
	fftw_free(mOut);
	// Julius解放
	delete mJuliusT;
}

void Record::setParent(QWidget *parent) {
	mParent = parent;
}


void Record::paintEvent(QPaintEvent *) {
	int w = ui.openGLWidget->width(), h = ui.openGLWidget->height();
	
	// ペンの準備
	QPainter painter(ui.openGLWidget);
	// painter.setRenderHint(QPainter::Antialiasing, true);//アンチエイリアスセット
	painter.setPen(QPen(Qt::white));
	painter.setBrush(QBrush(Qt::black));
	//painter.setFont(QFont("ＭＳ ゴシック", 9));

	painter.eraseRect(0, 0, w, h);
	painter.drawRect(0, 0, w, h);
	switch (mDispMode)
	{
	case 0:	// スペクトログラム
		{	
			painter.setPen((mRecNow) ? QColor(Qt::red) : QColor(Qt::white));
			float *amplitude = new float[FRAMES_PER_BUFFER];
			double f;
			int x = 0, x_ = 0;

			// FFTW実行
			for (int i = 0; i < FRAMES_PER_BUFFER; i++)
				mData[i][0] = mData[i][0] * (0.5 - 0.5 * cos(2 * pi * ((float)i / FRAMES_PER_BUFFER)));	// 窓関数
			fftw_execute(mPlan);

			// 描画
			for (int i = 0, j = 0; i < FRAMES_PER_BUFFER / 2; i++) {
				mOut[i][0] /= FRAMES_PER_BUFFER; mOut[i][1] /= FRAMES_PER_BUFFER;
				amplitude[i] = (mOut[i][0] * mOut[i][0] + mOut[i][1] * mOut[i][1]) * FRAMES_PER_BUFFER;
				f = (double)i * (Fs / FRAMES_PER_BUFFER);	// 周波数
				x = log10(f) * w * 0.2;	// 対数グラフにする（なってない）
				if (x != x_) {
					painter.drawLine(x, h / 2, x, h / 2 - amplitude[i] * ((float)h / 2.0));
					j++;

					if (j % 10 == 0)
						painter.drawText(x, h / 2 + (j / 10) * 10, QString::number((int)f));
				}
				x_ = x;
			}
			break;
		}
	case 1:	// wave表示
		{
			double x=0.0;
			for (auto d : mWaveData) {
				painter.setPen((d.rec) ? QColor(Qt::red) : QColor(Qt::white));
				painter.drawLine(x, h / 2, x, h / 2 - d.data * (h / 2));
				x += ((double)w / (double)mWaveData.size());
				if (x > w)break;
			}
		}
		break;
	case 2:	// 認識結果
		{		
			getWord();	// 認識結果取得
			int y=0;
			if (mWordData.size() * 10 < h) {
				for (auto d : mWordData) {
					painter.drawText(0, y, d);
					y += 10;
				}
			}
			else {
				y = h - 10;
				auto rv = mWordData;
				std::reverse(rv.begin(), rv.end());
				for (auto d : rv) {	//逆順
					if (y < 0)break;
					painter.drawText(0, y, d);
					y -= 10;
				}
			}
		}
		break;
	default:
		break;
	}

}


void Record::fileReference() {
	QString selFilter;
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("名前を付けて保存"),
		".",
		tr("音声ファイル(*.wav)"),
		&selFilter,
		QFileDialog::DontUseCustomDirectoryIcons
	);
	if (fileName.isEmpty()) {
		return;
	}
	ui.lineFileName->setText(fileName);
}

void Record::changeDispMode() {
	mDispMode++;
	mDispMode %= 3;
}

void Record::rec() {
	if (mRecNow) {	
		// 録音終了
		mRecNow = false;
		ui.recButton->setText(tr("録音(&R)"));

		// wave書き込み
		auto waverw = new waveRW();
		waverw->setSamplesPerSec(Fs);
		waverw->setDataChunkSize(mRecData.size() * 2);
		waverw->setLength(mRecData.size());
		waverw->setBitsPerSample(16);
		waverw->setBlockSize(16/8);
		waverw->setBytesPerSec(waverw->getBlockSize() * Fs);
		waverw->setRiffChunkSize(36 + waverw->getDataChunkSize());
		double *data = new double[mRecData.size()];
		long long int i=0;
		for (auto d : mRecData) {
			data[i] = d;
			i++;
		}
		waverw->setData(data);
		waverw->wave_write(ui.lineFileName->text().toLocal8Bit().constData());
		delete waverw;
		
		mRecData.clear();
		((KancopySaihu*)mParent)->setFileName(ui.lineFileName->text().toLocal8Bit().constData());
		this->close();
		QString str = "録音が完了しました。\n" + ui.lineFileName->text();
		QMessageBox::information(this, "", str);
	}
	else {
		// 録音開始
		mRecNow = true;
		ui.recButton->setText(tr("停止(&S)"));
	}
}

int Record::dsp(const void *inputBuffer, //入力
	void *outputBuffer, //出力
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo *timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData //ユーザ定義データ 
) {
	float *out = (float *)outputBuffer;
	float *in = (float *)inputBuffer;
	auto data = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FRAMES_PER_BUFFER);

	if (mWaveData.size() > framesPerBuffer * 10) mWaveData.erase(mWaveData.begin(), mWaveData.begin() + framesPerBuffer);

	long i;

	if (inputBuffer == NULL)
	{
		return 0;
	}

	for (i = 0; i < framesPerBuffer; i++) {
		data[i][0] = *in++;
		data[i][1] = 0;

		mWaveData.push_back(dWave{ data[i][0] , mRecNow });
		if (mRecNow) 
			mRecData.push_back(data[i][0]);
	}

	memcpy((fftw_complex*)userData, data, sizeof(fftw_complex) * FRAMES_PER_BUFFER);

	return 0;
}

void Record::getWord() {
	QString word;
	// qRegisterMetaType<std::string>("std::string");
	QMetaObject::invokeMethod(mJuliusT, "getResult", Qt::DirectConnection, Q_RETURN_ARG(QString, word)); // mJuliusT->getResult();
	if (mWordData.at(mWordData.size() - 1) != word) {
		mWordData.push_back(word);
	}
}
