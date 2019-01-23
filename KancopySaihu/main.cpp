#include <QtWidgets/QApplication>
#include <qicon.h>
#include "Control.h"

int main(int argc, char *argv[])
{
	// Qtアプリケーション作成
	QApplication a(argc, argv);

	// 初期化処理
	auto control = Control::create(); // new Control(argc,argv);
	control.init(argc, argv);
	
	a.setWindowIcon(QIcon("icon.png"));

	return a.exec();
}
