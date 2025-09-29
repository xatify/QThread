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
 	* @brief FIFO scheduling policy.
 	*
 	* Tasks are executed in the order they are submitted (First-In-First-Out).
 	* This is the default policy for the thread pool.
 	*
 	* @tparam T The type of element stored in the queue.
 	*/
	struct FiFoPolicy {
		template <typename T> using Container = std::queue<T>;
	};

	/**
	* @brief Priority scheduling policy.
	*
	* Tasks are scheduled according to a priority value.
	* Lower numeric values represent higher priority
	* (e.g., High = 0 executes before Normal = 1).
	*
	* @tparam T The type of element stored in the priority queue.
	*/
	struct PriorityPolicy {
		/**
    	* @brief Priority levels available for tasks.
    	*/
		enum class Priority { High = 0, Normal = 1, Low = 2 };

		template <typename T>
		using Container = std::priority_queue<T, std::vector<T>>;
		/**
		* @brief Wrapper for tasks when using a priority queue.
		*
		* Associates a callable task with a priority level so that
		* the thread pool can order tasks accordingly.
		*/
		struct TaskWrapper {
			std::function<void()>	 func;
			PriorityPolicy::Priority priority;

			bool operator<(const TaskWrapper& other) const {
				return static_cast<int>(priority) >
					   static_cast<int>(other.priority);
			}
		};
	};

	using Priority = PriorityPolicy::Priority;
	/**
	* @class ConcurrentQueue
	* @brief A thread-safe, blocking queue for inter-thread communication.
	*
	* Provides synchronized push/pop operations, allowing multiple producers
	* and consumers to safely exchange tasks or data. the underlying container
	* type is determnined by the scheduling Policy
	*
	* @tparam T			Type of element stored in the queue.
	* @tparam Policy	Scheduling policy for ordering elements (default: FiFo)
	*/
	template <typename T, typename Policy = FiFoPolicy> class ConcurrentQueue {
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

		inline void push(T item, PriorityPolicy::Priority prio);

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
		mutable std::mutex					   mtx_;
		std::condition_variable				   cv_;
		typename Policy::template Container<T> queue_;
	};

	/**
 	* @class ThreadPool
 	* @brief A simple, thread pool for parallel task execution.
 	*
 	* ThreadPool manages a set of worker threads that execute tasks
 	* submitted via submit(). Tasks can return results using std::future.
	* The scheduling policy (FIFO or Priority) is controlled via the template parameter.
	*
	* @tparam Policy Scheduling policy (default: FiFoPolicy)
 	*/
	template <typename Policy = FiFoPolicy> class ThreadPool {
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
		* @param priority Optional task priority (used only is Policy == PriorityPolicy)
    	* @return std::future<R> where R is the return type of f
    	*/
		template <typename F, typename... Args>
		auto submit(F&& f, Args&&... args)
			-> std::future<typename std::invoke_result<F, Args...>::type>;

		template <typename F, typename... Args>
		auto submit(F&& f, Args&&... args, PriorityPolicy::Priority)
			-> std::future<typename std::invoke_result<F, Args...>::type>;

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

		using TaskType =
			std::conditional_t<std::is_same_v<Policy, PriorityPolicy>,
							   typename PriorityPolicy::TaskWrapper,
							   std::function<void()>>;

		std::vector<std::thread>		  workers_;
		ConcurrentQueue<TaskType, Policy> taskQueue_;
		std::atomic<bool>				  stopFlag_;
		std::atomic<size_t>				  activeTasks_;
	};

	/**
 	* @brief Create a thread pool using FIFO (first-in, first-out) scheduling.
 	*
 	* Tasks are executed in the order they are submitted.
 	*
 	* @param n Number of worker threads (default: hardware concurrency or 2 if unknown).
 	* @return ThreadPool<FiFoPolicy> A FIFO thread pool instance.
 	*/
	inline ThreadPool<FiFoPolicy> make_fifo_pool(size_t n = 0) {
		return ThreadPool<FiFoPolicy>(n);
	}

	/**
	* @brief Create a thread pool using priority-based scheduling.
	*
	* Tasks can be submitted with a PriorityPolicy::Priority value.
	* Lower numeric values indicate higher priority.
	*
	* @param n Number of worker threads (default: hardware concurrency or 2 if unknown).
	* @return ThreadPool<PriorityPolicy> A priority-based thread pool instance.
	*/
	inline ThreadPool<PriorityPolicy> make_priority_pool(size_t n = 0) {
		return ThreadPool<PriorityPolicy>(n);
	}

	template <typename T, typename Policy>
	inline void ConcurrentQueue<T, Policy>::push(T item, Priority prio) {
		{
			std::lock_guard<std::mutex> lock(mtx_);
			queue_.push({std::move(item), prio});
		}
		cv_.notify_one();
	}

	template <typename T, typename Policy>
	inline void ConcurrentQueue<T, Policy>::push(T item) {
		{
			std::lock_guard<std::mutex> lock(mtx_);
			queue_.push(std::move(item));
		}
		cv_.notify_one();
	}

	template <typename T, typename Policy>
	inline std::optional<T> ConcurrentQueue<T, Policy>::pop() {
		std::unique_lock<std::mutex> lock(mtx_);
		cv_.wait(lock, [this] { return !queue_.empty(); });

		if (queue_.empty())
			return std::nullopt;

		if constexpr (std::is_same_v<Policy, PriorityPolicy>) {
			auto wrapper = std::move(queue_.top());
			queue_.pop();
			return wrapper;
		} else {
			auto item = std::move(queue_.front());
			queue_.pop();
			return (item);
		}
	}

	template <typename T, typename Policy>
	inline bool ConcurrentQueue<T, Policy>::empty() const {
		std::lock_guard<std::mutex> lock(mtx_);
		return queue_.empty();
	}

	template <typename Policy>
	inline ThreadPool<Policy>::ThreadPool(size_t numThreads)
		: stopFlag_(false), activeTasks_(0) {
		if (numThreads == 0) {
			numThreads = std::thread::hardware_concurrency();
			if (numThreads == 0)
				numThreads = 2;
		}

		for (size_t i = 0; i < numThreads; i++) {
			workers_.emplace_back(&ThreadPool::worker_loop, this);
		}
	}

	template <typename Policy> inline ThreadPool<Policy>::~ThreadPool() {
		shutdown();
	}

	template <typename Policy> inline void ThreadPool<Policy>::worker_loop() {
		while (!stopFlag_) {
			auto taskOpt = taskQueue_.pop();

			// if shutdown and no tasks
			if (!taskOpt.has_value()) {
				if (stopFlag_)
					return;
				continue;
			}

			if constexpr (std::is_same_v<Policy, PriorityPolicy>) {
				auto wrapper = std::move(taskOpt.value());
				activeTasks_++;
				try {
					wrapper.func();
				} catch (...) {
				}
				activeTasks_--;
			} else {
				auto task = std::move(taskOpt.value());
				activeTasks_++;
				try {
					task();
				} catch (...) {
				}
				activeTasks_--;
			}
		}
	}

	template <typename Policy>
	template <typename F, typename... Args>
	auto ThreadPool<Policy>::submit(F&& f, Args&&... args)
		-> std::future<typename std::invoke_result<F, Args...>::type> {
		using return_type = typename std::invoke_result<F, Args...>::type;

		auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		taskQueue_.push([taskPtr]() { (*taskPtr)(); });
		std::future<return_type> result = taskPtr->get_future();
		return (result);
	}

	template <typename Policy>
	template <typename F, typename... Args>
	auto ThreadPool<Policy>::submit(F&& f, Args&&... args, Priority priority)
		-> std::future<typename std::invoke_result<F, Args...>::type> {
		using return_type = typename std::invoke_result<F, Args...>::type;

		auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		auto wrapper = typename PriorityPolicy::TaskWrapper{
			[taskPtr]() { (*taskPtr)(); }, priority};
		taskQueue_.push(std::move(wrapper));
		std::future<return_type> result = taskPtr->get_future();
		return (result);
	}

	template <typename Policy> inline void ThreadPool<Policy>::shutdown() {
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

	template <typename Policy>
	inline void ThreadPool<Policy>::wait_for_completion() {
		while (activeTasks_ > 0 || !taskQueue_.empty()) {
			std::this_thread::sleep_for(std::chrono::microseconds(50));
		}
	}

}; // namespace qthread

#endif
