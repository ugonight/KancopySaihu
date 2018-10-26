#pragma once

#include <QtCore>
#include <string>

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

private:
	static void output_result(struct __Recog__ *recog, void *dummy);

	struct __Jconf__ *mJconf;
	struct __Recog__ *mRecog;
	static QString mResult;
};