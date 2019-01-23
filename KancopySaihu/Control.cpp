#pragma execution_character_set("utf-8")

#include "Control.h"
#include "KancopySaihu.h"
#include <UUtauData.h>

Control* Control::instance = nullptr;

//Control::Control(int argc, char *argv[]) {
//	// TUtauDataのインスタンスを作成し、パイプファイルを読み込む。
//	mUtauData = new TUtauData();
//	if (mUtauData->SetTo(argv[1]) != B_OK) {
//		qDebug("パイプファイルを読み込めません。");
//		return;
//	}
//
//	mUtauFileName = argv[1];
//	instance = this;
//}

Control::Control()
{
	instance = this;
}


Control::~Control()
{
	delete mUtauData;
	delete mMainWindow;
}

Control& Control::get_instance()
{
	return *instance;
}

Control& Control::create()
{
	if (!instance) {
		instance = new Control;
	}

	return *instance;
}

void Control::destroy()
{
	delete instance;
	instance = nullptr;
}


void Control::init(int argc, char *argv[]) {
	// TUtauDataのインスタンスを作成し、パイプファイルを読み込む。
	mUtauData = new TUtauData();
	if (mUtauData->SetTo(argv[1]) != B_OK) {
		qDebug("パイプファイルを読み込めません。");
		// qApp->exit();
		return;
	}

	mUtauFileName = argv[1];
	instance = this;


	// 設定画面表示
	mMainWindow = new KancopySaihu();
	mMainWindow->show();
	double tempo;
	mUtauData->SectionSettings()->GetValue(KEY_NAME_TEMPO, &tempo);
	mMainWindow->setTempo(tempo);
}

void Control::exportUtau() {
	mUtauData->Export(mUtauFileName.toStdString());
	// qApp->exit();
}


TUtauData* Control::getUtauData() {
	return mUtauData;
}


