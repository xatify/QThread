#include "qthread.hpp"
#include <cassert>
#include <iostream>

int main() {
	qthread::ThreadPool pool(4);

	auto f1 = pool.submit([] { return 1 + 1; });
	auto f2 = pool.submit([](int x) { return x * 2; }, 21);

	pool.wait_for_completion();

	assert(f1.get() == 2);
	assert(f2.get() == 42);

	std::cout << "All tests passed\n";
	return 0;
}
