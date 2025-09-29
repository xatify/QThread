#include "qthread.hpp"
#include <cassert>
#include <iostream>

#include <chrono>

using namespace std::chrono_literals;

int main() {
	// --- Test FIFO pool ---
	{
		auto pool = qthread::make_fifo_pool(4);

		auto f1 = pool.submit([] { return 1 + 1; });
		auto f2 = pool.submit([](int x) { return x * 2; }, 21);

		pool.wait_for_completion();

		assert(f1.get() == 2);
		assert(f2.get() == 42);

		std::cout << "FIFO pool test passed\n";
	}

	// --- Test Priority pool ---
	{
		auto	   pool = qthread::make_priority_pool(1);
		std::mutex mtx;

		auto record = [&](int i, int ms) {
			return [&, i, ms] { std::cout << "p: " << i << std::endl; };
		};

		using Priority = qthread::PriorityPolicy::Priority;

		for (int i = 0; i < 100; i++) {
			auto p = static_cast<Priority>(i % 3);
			pool.submit(record(i % 3, 10), p);
		}

		pool.wait_for_completion();
	}
	return 0;
}
