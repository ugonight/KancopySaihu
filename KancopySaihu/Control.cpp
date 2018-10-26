#include "Control.h"
#include "KancopySaihu.h"
#include<UUtauData.h>

Control::Control(int argc, char *argv[]) {
	// TUtauDataのインスタンスを作成し、パイプファイルを読み込む。
	mUtauData = new TUtauData();
	if (mUtauData->SetTo(argv[1]) != B_OK) {
		qDebug("パイプファイルを読み込めません。");
		return;
	}
}

Control::Control()
{
	
}


Control::~Control()
{
	delete mUtauData;
}

void Control::init() {
	// 設定画面表示
	mMainWindow = new KancopySaihu();
	mMainWindow->show();
	double tempo;
	mUtauData->SectionSettings()->GetValue(KEY_NAME_TEMPO, &tempo);
	mMainWindow->setTempo(tempo);
}


