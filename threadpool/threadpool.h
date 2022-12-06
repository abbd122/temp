#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <QDebug>
#include <QObject>
#include <QThread>
#include <queue>
#include <vector>

using TaskFunc = std::function<void()>;

class Worker : public QThread {
	Q_OBJECT

public:
	explicit Worker(std::queue<TaskFunc>& tasks, std::mutex& mtx, std::condition_variable& cv,
	                bool& stop);

	void run() override;

private:
	bool& m_stop;

	std::queue<TaskFunc>& m_tasks;

	std::mutex& m_mtx;

	std::condition_variable& m_cv;

signals:
	void finished();
};

class ThreadPool {
public:
	explicit ThreadPool(size_t threadNumber);

	~ThreadPool();

	template <typename Fn, typename... Args>
	std::future<typename std::result_of_t<Fn(Args ...)>> DoTask(Fn&& f, Args&&... args);

	void Stop();

private:
	std::vector<Worker*> m_workers{};

	std::queue<TaskFunc> m_tasks{};

	std::mutex m_mtx{};

	std::condition_variable m_cv{};

	bool m_stop{};
};

inline Worker::Worker(std::queue<TaskFunc>& tasks, std::mutex& mtx, std::condition_variable& cv,
                      bool& stop) :
	m_stop(stop), m_tasks(tasks), m_mtx(mtx), m_cv(cv)
{
}

inline void Worker::run()
{
	for (;;) {
		std::unique_lock<std::mutex> lk(m_mtx);
		m_cv.wait(lk, [this] { return m_stop || !m_tasks.empty(); });
		if (m_stop) {
			qDebug().noquote() << "stop";
			if (!m_tasks.empty()) {
				qDebug().noquote() << "Some tasks are not completed";
			}
			return;
		}
		TaskFunc task = std::move(m_tasks.front());
		m_tasks.pop();
		lk.unlock();
		task();
		emit finished();
	}
}

inline ThreadPool::ThreadPool(size_t threadNumber)
{
	for (size_t i = 0; i < threadNumber; ++i) {
		m_workers.push_back(new Worker(m_tasks, m_mtx, m_cv, m_stop));
	}
	for (const auto& worker : m_workers) {
		worker->start();
	}
}

inline ThreadPool::~ThreadPool()
{
	Stop();
}

inline void ThreadPool::Stop()
{
	std::unique_lock<std::mutex> lk(m_mtx);
	m_stop = true;
	m_cv.notify_all();
	for (const auto& worker : m_workers) {
		worker->deleteLater();
	}
}

template <typename Fn, typename... Args>
std::future<typename std::result_of_t<Fn(Args ...)>> ThreadPool::DoTask(Fn&& f, Args&&... args)
{
	using ReturnType = typename std::result_of_t<Fn(Args ...)>;
	auto task = std::make_shared<std::packaged_task<ReturnType()>>(
		std::bind(std::forward<Fn>(f), std::forward<Args>(args)...));
	std::future<ReturnType> res = task->get_future();
	std::unique_lock<std::mutex> (m_mtx);
	if (m_stop) {
		qDebug().noquote() << "threadpool has stop";
		return {};
	}
	m_tasks.emplace([task] {
		(*task)();
	});
	m_cv.notify_one();
	return res;
}
