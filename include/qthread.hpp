#ifndef __QTHREAD_HPP__
#define __QTHREAD_HPP__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace qthread {

	/**
	* @class ConcurrentQueue
	* @brief A thread-safe, blocking queue for inter-thread communication.
	*
	* Provides synchronized push/pop operations, allowing multiple producers
	* and consumers to safely exchange tasks or data.
	*
	* @tparam T Type of element stored in the queue.
	*/
	template <typename T> class ConcurrentQueue {
	  public:
		ConcurrentQueue()  = default;
		~ConcurrentQueue() = default;

		ConcurrentQueue(const ConcurrentQueue&)			   = delete;
		ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

		/**
    	* @brief Push an item into the queue.
    	* @param item The item to insert.
    	*
    	* Thread-safe, notifies one waiting consumer.
    	*/
		void push(T item);

		/**
    	* @brief Pop an item from the queue (blocking).
    	*
    	* Blocks until an item is available. Returns std::nullopt
    	* if the queue is empty after being awakened (e.g. during shutdown).
    	*
    	* @return std::optional<T> The next item, or nullopt if empty.
    	*/
		std::optional<T> pop();

		/**
     	* @brief Check if the queue is empty.
     	* @return true if empty, false otherwise.
     	*/
		bool empty() const;

	  private:
		mutable std::mutex		mtx_;
		std::condition_variable cv_;
		std::queue<T>			queue_;
	};

	/**
 	* @class ThreadPool
 	* @brief A simple, thread pool for parallel task execution.
 	*
 	* ThreadPool manages a set of worker threads that execute tasks
 	* submitted via submit(). Tasks can return results using std::future.
 	*/
	class ThreadPool {
	  public:
		/**
    	* @brief Construct a thread pool.
    	* @param numThreads Number of worker threads (defaults to hardware concurrency).
    	*/
		explicit ThreadPool(size_t numThreads = 0);
		~ThreadPool();

		ThreadPool(const ThreadPool&)			 = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		/**
    	* @brief Submit a task to the pool.
    	*
    	* The task is executed asynchronously by the next available worker.
    	* Returns a future representing the result.
    	*
    	* @tparam F Callable type
    	* @tparam Args Argument types
    	* @param f Callable object
    	* @param args Arguments to pass to f
    	* @return std::future<R> where R is the return type of f
    	*/
		template <typename F, typename... Args>
		auto submit(F&& f,
					Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

		/**
    	* @brief Gracefully stop the pool.
    	*
    	* No new tasks will be executed. Waits for all workers to finish.
    	*/
		void shutdown();

		/**
    	* @brief Block until all tasks are complete.
    	*
    	* Unlike shutdown(), workers remain active and can accept new tasks.
    	*/
		void wait_for_completion();

	  private:
		// Worker thread loop
		void worker_loop();

		std::vector<std::thread>			   workers_;
		ConcurrentQueue<std::function<void()>> taskQueue_;
		std::atomic<bool>					   stopFlag_;
		std::atomic<size_t>					   activeTasks_;
	};

	template <typename T> inline void ConcurrentQueue<T>::push(T item) {
		{
			std::lock_guard<std::mutex> lock(mtx_);
			queue_.push(std::move(item));
		}
		cv_.notify_one();
	}

	template <typename T> inline std::optional<T> ConcurrentQueue<T>::pop() {
		std::unique_lock<std::mutex> lock(mtx_);
		cv_.wait(lock, [this] { return !queue_.empty(); });

		if (queue_.empty())
			return std::nullopt;

		T item = std::move(queue_.front());
		queue_.pop();
		return (item);
	}

	template <typename T> inline bool ConcurrentQueue<T>::empty() const {
		std::lock_guard<std::mutex> lock(mtx_);
		return queue_.empty();
	}

	inline ThreadPool::ThreadPool(size_t numThreads) : stopFlag_(false), activeTasks_(0) {
		if (numThreads == 0) {
			numThreads = std::thread::hardware_concurrency();
			if (numThreads == 0)
				numThreads == 2;
		}

		for (size_t i = 0; i < numThreads; i++) {
			workers_.emplace_back(&ThreadPool::worker_loop, this);
		}
	}

	inline ThreadPool::~ThreadPool() {
		shutdown();
	}

	inline void ThreadPool::worker_loop() {
		while (!stopFlag_) {
			auto taskOpt = taskQueue_.pop();

			// if shutdown and no tasks
			if (!taskOpt.has_value()) {
				if (stopFlag_)
					return;
				continue;
			}

			auto task = std::move(taskOpt.value());
			activeTasks_++;
			try {
				task();
			} catch (...) {
				// swallow excepts
			}
			activeTasks_--;
		}
	}

	template <typename F, typename... Args>
	auto ThreadPool::submit(F&& f, Args&&... args)
		-> std::future<typename std::invoke_result<F, Args...>::type> {
		using return_type = typename std::invoke_result<F, Args...>::type;

		auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::future<return_type> result = taskPtr->get_future();

		taskQueue_.push([taskPtr]() { (*taskPtr)(); });

		return (result);
	}

	inline void ThreadPool::shutdown() {
		stopFlag_ = true;

		// wake up all workers so they can exit
		for (size_t i = 0; i < workers_.size(); ++i) {
			taskQueue_.push({});
		}

		for (auto& w : workers_) {
			if (w.joinable()) {
				w.join();
			}
		}
	}

	inline void ThreadPool::wait_for_completion() {
		while (activeTasks_ > 0 || !taskQueue_.empty()) {
			std::this_thread::sleep_for(std::chrono::microseconds(50));
		}
	}

}; // namespace qthread

#endif
