#include <QtWidgets/QApplication>
#include "Control.h"

int main(int argc, char *argv[])
{
	// Qt�A�v���P�[�V�����쐬
	QApplication a(argc, argv);

	// ����������
	auto control = new Control(argc,argv);
	control->init();
	
	return a.exec();
}
