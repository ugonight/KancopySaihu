#pragma once

#include "qstring.h"

class TUtauData;
class KancopySaihu;

// �S�̂��Ǘ�����N���X

class Control
{
private:
	TUtauData *mUtauData;		// LibUTAU�̃t�@�C�����Ǘ�����N���X
	KancopySaihu *mMainWindow;	// �ŏ��̃E�B���h�E

	QString mUtauFileName;

	static Control* instance;

public:
	// Control(int argc, char *argv[]);
	Control();
	~Control();

	static Control& get_instance();
	static Control& create();
	static void destroy();

	void init(int argc, char *argv[]);
	void exportUtau();

	TUtauData* getUtauData();
};

