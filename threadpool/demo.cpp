#include "demo.h"

Demo::Demo(ThreadPool& pool, QWidget* parent)
	: QWidget(parent), m_pool(pool)
{
	ui.setupUi(this);
}

int Demo::Task(int num)
{
	QThread::sleep(3);
	qDebug().noquote() << num;
	return num;
}

void Demo::PushBottonClicked()
{
	qDebug().noquote() << "click";
	auto res = m_pool.DoTask([this](int num) { return Task(num); }, m_num++);
	// qDebug().noquote() << res.get();
}
