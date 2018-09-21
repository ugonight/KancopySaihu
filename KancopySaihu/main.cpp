#include <QtWidgets/QApplication>
#include "Control.h"

int main(int argc, char *argv[])
{
	// Qtアプリケーション作成
	QApplication a(argc, argv);

	// 初期化処理
	auto control = new Control(argc,argv);
	control->init();
	
	return a.exec();
}
