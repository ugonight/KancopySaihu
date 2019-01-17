#pragma once

#include <QtCore>
#include <string>

typedef std::tuple<float**, int, int> mfcc_tuple;


// JuliusLibを使うためのクラス
class JuliusT : public QObject
{
	Q_OBJECT

public:

	JuliusT(QObject *parent = 0);	// マイク入力
	//JuliusT(std::string filelist, QObject *parent = 0);	// ファイル入力

	~JuliusT();


//signals:
	//void progress(int);     //  処理進行状況通知

public slots:
	void init();
	void init(std::string filelist);

	void startRecog();		// 認識開始
	QString getResult();
	mfcc_tuple getMfccResult();
	void setDivMode(bool);

private:
	static void output_result(struct __Recog__ *recog, void *dummy);

	struct __Jconf__ *mJconf;
	struct __Recog__ *mRecog;
	static QString mResult;
	static mfcc_tuple mMfccResult;
	static bool mDivMode;
	static int mDivId;
};