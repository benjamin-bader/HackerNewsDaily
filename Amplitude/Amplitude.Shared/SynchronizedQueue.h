#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

namespace Amplitude
{
	using std::condition_variable;
	using std::deque;
	using std::function;
	using std::mutex;
	using std::unique_lock;

	// A simple blocking queue.  It is intended for multi-producer,
	// single-consumer scenarios, e.g. a thread event-loop.
	template <typename T>
	class SynchronizedQueue
	{
	public:
		SynchronizedQueue(unsigned int size = 1024);
		~SynchronizedQueue();

		//void Enqueue(const T &item);
		//void Enqueue(T&& item);

		bool TryEnqueue(const T &item);
		bool TryEnqueue(T&& item);

		bool TryDequeue(T &item);

		void Complete();

	private:
		const unsigned int size;

		bool is_complete;
		deque<T> queue;
		mutex queue_mutex;
		condition_variable empty;
	};

	template <typename T>
	SynchronizedQueue<T>::SynchronizedQueue(unsigned int size)
		: is_complete(false), size(size)
	{
	}

	template <typename T>
	SynchronizedQueue<T>::~SynchronizedQueue()
	{
		unique_lock<mutex> lock(queue_mutex);
		queue.clear();
		Complete();
	}

	template <typename T>
	bool SynchronizedQueue<T>::TryEnqueue(const T &item)
	{
		auto didEnqueue = false;

		unique_lock<mutex> lock(queue_mutex);
		if (!is_complete)
		{
			if (queue.size() < this->size)
			{
				queue.push_back(item);
				didEnqueue = true;
			}
		}
		lock.unlock();

		if (didEnqueue)
		{
			empty.notify_one();
		}

		return didEnqueue;
	}

	template <typename T>
	bool SynchronizedQueue<T>::TryEnqueue(T&& item)
	{
		return TryEnqueue(std::move(item));
	}

	template <typename T>
	bool SynchronizedQueue<T>::TryDequeue(T &item)
	{
		auto result = false;

		unique_lock<mutex> lock(queue_mutex);
		while (queue.empty() && !is_complete)
		{
			empty.wait(lock);
		}

		if (!queue.empty())
		{
			item = queue.front();
			queue.pop_front();
			result = true;
		}

		return result;
	}

	template <typename T>
	void SynchronizedQueue<T>::Complete()
	{
		unique_lock<mutex> lock(queue_mutex);
		is_complete = true;
	}
}