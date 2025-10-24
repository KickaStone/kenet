#include "../../include/threadpool/threadpool.h"

#include <iostream>
#include <vector>

int main() {
    ThreadPool pool(4ul);
    std::vector<std::future<int>> results;

    for (size_t i = 0;i<10;++i) {
        results.emplace_back(
            pool.enqueue([i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::cout << "Task " << i << " is running in thread "
                << std::this_thread::get_id() << std::endl;
                return i*i;
            })
        );
    }

    for (auto && result : results) {
        std::cout << "Result: " << result.get() << std::endl;
    }
}
