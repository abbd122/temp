#include <QtWidgets/QApplication>

#include "demo.h"
#include "threadpool.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	ThreadPool pool(1);
	return a.exec();
}
