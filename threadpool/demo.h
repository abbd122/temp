#pragma once

#include <QtWidgets/QWidget>
#include <QList>

#include "threadpool.h"
#include "ui_demo.h"

class Demo : public QWidget {
	Q_OBJECT

public:
	Demo(ThreadPool& pool, QWidget* parent = nullptr);

	~Demo() override = default;

private:
	Ui::demoClass ui;

	ThreadPool& m_pool;

	int m_num{};

	int Task(int num);

public slots:
	void PushBottonClicked();
};
