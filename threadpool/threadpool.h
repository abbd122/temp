#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <QObject>
#include <QThread>
#include <queue>
#include <vector>

// https://github.com/progschj/ThreadPool/blob/master/ThreadPool.h
using TaskFunc = std::function<void()>;

class Worker : public QThread {
	Q_OBJECT

public:
	void run() override;

private:
	bool m_stopRequest{};

	std::unique_ptr<TaskFunc> m_currTask{};

	std::unique_ptr<TaskFunc> m_nextTask{};

	std::mutex m_mtx{};

	std::condition_variable m_cv{};

signals:
	void finished();
};

class ThreadPool {
public:
	explicit ThreadPool(size_t threadNumber);

private:
	std::vector<Worker*> m_workers{};

	std::queue<TaskFunc> m_tasks{};
};
