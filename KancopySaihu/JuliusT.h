#pragma once

#include <QtCore>
#include <string>

typedef std::tuple<float**, int, int> mfcc_tuple;


// JuliusLib���g�����߂̃N���X
class JuliusT : public QObject
{
	Q_OBJECT

public:

	JuliusT(QObject *parent = 0);	// �}�C�N����
	//JuliusT(std::string filelist, QObject *parent = 0);	// �t�@�C������

	~JuliusT();


//signals:
	//void progress(int);     //  �����i�s�󋵒ʒm

public slots:
	void init();
	void init(std::string filelist);

	void startRecog();		// �F���J�n
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