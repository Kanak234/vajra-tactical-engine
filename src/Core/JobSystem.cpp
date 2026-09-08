#include "Core/JobSystem.h"

#include <algorithm>

namespace vajra {

void JobSystem::start(unsigned threadCount) {
    if (m_running) return;
    if (threadCount == 0) {
        const unsigned hardware = std::thread::hardware_concurrency();
        // Leave one core for the main thread and the driver.
        threadCount = hardware > 2 ? hardware - 1 : 1;
    }

    m_running = true;
    m_workers.reserve(threadCount);
    for (unsigned i = 0; i < threadCount; ++i)
        m_workers.emplace_back([this] { workerLoop(); });
}

void JobSystem::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_running) return;
        m_running = false;
    }
    m_wake.notify_all();
    for (std::thread& worker : m_workers)
        if (worker.joinable()) worker.join();
    m_workers.clear();
}

void JobSystem::workerLoop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [this] { return !m_running || !m_queue.empty(); });
            if (!m_running && m_queue.empty()) return;
            job = std::move(m_queue.front());
            m_queue.pop();
        }
        job();
        if (m_pending.fetch_sub(1) == 1) m_done.notify_all();
    }
}

void JobSystem::parallelFor(size_t count, size_t minChunk,
                            const std::function<void(size_t, size_t)>& body) {
    if (count == 0) return;

    // Below the threshold, threading costs more than it saves.
    if (m_workers.empty() || count <= minChunk) {
        body(0, count);
        return;
    }

    const size_t chunks = std::min(count / minChunk, static_cast<size_t>(m_workers.size()) + 1);
    const size_t chunkSize = (count + chunks - 1) / chunks;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t begin = chunkSize; begin < count; begin += chunkSize) {
            const size_t end = std::min(begin + chunkSize, count);
            m_pending.fetch_add(1);
            m_queue.emplace([&body, begin, end] { body(begin, end); });
        }
    }
    m_wake.notify_all();

    // The calling thread takes the first chunk rather than idling.
    body(0, std::min(chunkSize, count));

    std::unique_lock<std::mutex> lock(m_mutex);
    m_done.wait(lock, [this] { return m_pending.load() == 0; });
}

}  // namespace vajra
