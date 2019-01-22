#pragma once

#include "qstring.h"

class TUtauData;
class KancopySaihu;

// 全体を管理するクラス

class Control
{
private:
	TUtauData *mUtauData;		// LibUTAUのファイルを管理するクラス
	KancopySaihu *mMainWindow;	// 最初のウィンドウ

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

