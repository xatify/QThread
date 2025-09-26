#include "qthread.hpp"
#include <atomic>
#include <cassert>
#include <future>
#include <iostream>

using namespace qthread;

// Simple recursive tree builder
struct TreeTask {
	int								  depth;
	std::shared_ptr<std::atomic<int>> counter;
	ThreadPool*						  pool;

	void operator()() {
		counter->fetch_add(1);

		if (depth > 0) {
			pool->submit(TreeTask{depth - 1, counter, pool});
			pool->submit(TreeTask{depth - 1, counter, pool});
		}
	}
};

int main() {
	ThreadPool pool(4);

	auto counter = std::make_shared<std::atomic<int>>(0);
	int	 depth	 = 5;

	// Root task
	auto future = pool.submit(TreeTask{depth, counter, &pool});

	future.get();

	// Give workers time to finish recursive tasks
	pool.wait_for_completion();

	int expected_nodes = (1 << (depth + 1)) - 1; // 2^(depth+1) - 1
	std::cout << "Created " << *counter << " nodes, expected " << expected_nodes
			  << "\n";
	assert(*counter == expected_nodes);

	std::cout << "Tree test passed!" << std::endl;
	return 0;
}
