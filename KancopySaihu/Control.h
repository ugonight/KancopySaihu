#pragma once

class TUtauData;
class KancopySaihu;

// �S�̂��Ǘ�����N���X

class Control
{
private:
	TUtauData *mUtauData;		// LibUTAU�̃t�@�C�����Ǘ�����N���X
	KancopySaihu *mMainWindow;	// �ŏ��̃E�B���h�E


public:
	Control(int argc, char *argv[]);
	Control();
	~Control();

	void init();
};

