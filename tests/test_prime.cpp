#include "qthread.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace qthread;

// Simple prime checker
bool is_prime(unsigned int n) {
	if (n < 2)
		return false;
	for (unsigned int i = 2; i <= std::sqrt(n); i++) {
		if (n % i == 0)
			return false;
	}
	return true;
}

int main() {
	ThreadPool pool(4);

	std::vector<unsigned int>	   numbers = {2, 3, 4, 5, 17, 18, 19, 20, 97, 99};
	std::vector<std::future<bool>> futures;

	// Submit tasks
	for (auto n : numbers) {
		futures.push_back(pool.submit([n] { return is_prime(n); }));
	}

	// Collect results
	std::vector<bool> results;
	for (auto& f : futures) {
		results.push_back(f.get());
	}

	// Expected results
	std::vector<bool> expected = {true, true, false, true, true, false, true, false, true, false};

	// Validate
	assert(results == expected);
	std::cout << "✅ Prime test passed!" << std::endl;

	return 0;
}
