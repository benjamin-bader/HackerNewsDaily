#include "pch.h"
#include "SynchronizedQueue.h"
#include "WorkerThread.h"

#include <thread>

using namespace Amplitude;

typedef SynchronizedQueue<function<void()>> WorkQueue;

class WorkerThread::Impl
{
public:
	Impl();
	~Impl();

	Impl(Impl const&) = delete;
	Impl(Impl&&) = delete;
	Impl& operator=(Impl const&) = delete;

	void Start();
	bool TryAddWorkItem(function<void()> item);

private:
	std::atomic<bool> running;
	std::thread thread;

	// A queue of work items; they will be processed in
	// FIFO order.
	WorkQueue queue;

	void ProcessQueue();
};

WorkerThread::Impl::Impl() : queue(), thread()
{
}

WorkerThread::Impl::~Impl()
{
	running.store(false);
	queue.Complete();
	try
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
	catch (const std::exception& ex)
	{
		LogDebug(ex.what());
	}
	catch (Platform::Exception ^ex)
	{
		LogDebug(ex->ToString()->Data());
	}
	catch (...)
	{
		LogDebug("CRITICAL ERROR: Something was thrown from a work queue, but was not an exception!!!");
	}
}

void
WorkerThread::Impl::Start()
{
	running.store(true);
	thread = std::thread(&Impl::ProcessQueue, this);
}

bool
WorkerThread::Impl::TryAddWorkItem(function<void()> item)
{
	return running.load() && queue.TryEnqueue(item);
}

void
WorkerThread::Impl::ProcessQueue()
{
	function<void()> fn;
	while (queue.TryDequeue(fn))
	{
		try
		{
			fn();
		}
		catch (Platform::Exception ^ex)
		{
			LogDebug(ex->Message->Data());
		}
		catch (const std::exception &ex)
		{
			LogDebug(ex.what());
		}
		catch (...)
		{
			LogDebug("neither fish nor fowl.");
		}
	}
}

WorkerThread::WorkerThread() : impl(std::make_unique<Impl>())
{
	impl->Start();
}

WorkerThread::~WorkerThread()
{
}

bool
WorkerThread::TryAddWorkItem(function<void()> item)
{
	return impl->TryAddWorkItem(item);
}
