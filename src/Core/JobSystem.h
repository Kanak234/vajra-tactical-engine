#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace vajra {

/// Minimal work-stealing-free thread pool with a parallel-for. Deliberately
/// small: a game this size has exactly one workload that benefits from
/// threading (particle simulation), and a simple pool beats a complex
/// scheduler you cannot reason about at 3am.
class JobSystem {
public:
    void start(unsigned threadCount = 0);
    void stop();
    ~JobSystem() { stop(); }

    /// Splits [0, count) into chunks across the pool and blocks until done.
    void parallelFor(size_t count, size_t minChunk,
                     const std::function<void(size_t begin, size_t end)>& body);

    [[nodiscard]] unsigned workerCount() const { return static_cast<unsigned>(m_workers.size()); }

private:
    void workerLoop();

    std::vector<std::thread>          m_workers;
    std::queue<std::function<void()>> m_queue;
    std::mutex                        m_mutex;
    std::condition_variable           m_wake;
    std::condition_variable           m_done;
    std::atomic<int>                  m_pending{0};
    bool                              m_running = false;
};

}  // namespace vajra
