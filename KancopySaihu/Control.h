#pragma once

class TUtauData;
class KancopySaihu;

// 全体を管理するクラス

class Control
{
private:
	TUtauData *mUtauData;		// LibUTAUのファイルを管理するクラス
	KancopySaihu *mMainWindow;	// 最初のウィンドウ


public:
	Control(int argc, char *argv[]);
	Control();
	~Control();

	void init();
};

