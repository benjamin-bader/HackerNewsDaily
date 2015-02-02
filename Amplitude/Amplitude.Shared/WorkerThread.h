#pragma once

#include <functional>
#include <memory>

namespace Amplitude
{
	class WorkerThread
	{		
	public:
		WorkerThread();
		~WorkerThread();

		WorkerThread(WorkerThread const&) = delete;
		WorkerThread(WorkerThread&&) = delete;
		WorkerThread& operator=(WorkerThread const&) = delete;

		bool TryAddWorkItem(std::function<void()> item);

	private:
		class Impl;
		std::unique_ptr<Impl> impl;
	};
}