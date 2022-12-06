#include "threadpool.h"

#include <QDebug>

void Worker::run()
{
	for (;;) {
		std::unique_lock<std::mutex> lk(m_mtx);
		m_currTask = nullptr;
		m_cv.notify_all();
		m_cv.wait(lk, [this] { return m_stopRequest || m_nextTask; });
		if (m_stopRequest) {
			return;
		}
		m_currTask = std::move(m_nextTask);
		lk.unlock();
		(*m_currTask)();
		emit finished();
	}
}

ThreadPool::ThreadPool(size_t threadNumber)
{
	for (size_t i = 0; i < threadNumber; ++i) {
		m_workers.push_back(new Worker());
	}
	for (const auto& worker : m_workers) {
		worker->start();
	}
}
