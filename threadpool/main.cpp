#include <QtWidgets/QApplication>

#include "demo.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	ThreadPool pool(4);
	Demo demo(pool);
	demo.show();
	return a.exec();
}
