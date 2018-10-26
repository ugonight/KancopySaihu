#include "Control.h"
#include "KancopySaihu.h"
#include<UUtauData.h>

Control::Control(int argc, char *argv[]) {
	// TUtauData�̃C���X�^���X���쐬���A�p�C�v�t�@�C����ǂݍ��ށB
	mUtauData = new TUtauData();
	if (mUtauData->SetTo(argv[1]) != B_OK) {
		qDebug("�p�C�v�t�@�C����ǂݍ��߂܂���B");
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
	// �ݒ��ʕ\��
	mMainWindow = new KancopySaihu();
	mMainWindow->show();
	double tempo;
	mUtauData->SectionSettings()->GetValue(KEY_NAME_TEMPO, &tempo);
	mMainWindow->setTempo(tempo);
}


