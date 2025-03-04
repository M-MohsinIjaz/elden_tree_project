#ifndef ELDEN_TREE_EVENT_HANDLER_H
#define ELDEN_TREE_EVENT_HANDLER_H

#include <functional>
#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>

namespace elden_tree_ns {

    // using alias for event handler tamplate
    template<typename EventType>
    using EventHandler = std::function<void(const EventType&)>;

    // This class template allows registration of event handlers to Gods, each God can register 
    // multiple handlers 
    template<typename EventType>
    class EventMessenger {
    public:
        using GodId = std::string;

        // Register an event handler for a given god.
        void registerHandler(const GodId& god, EventHandler<EventType> handler) {
            // take lock so that multiple threads don't cause race condition
            std::lock_guard<std::mutex> lock(mutex_);
            // add handler to respective god's event handler's vector 
            // here vector is used because it uses contiguos memory and performace of push_back is better because of 
            // pre-allocated memory
            handlers_[god].push_back(handler);
        }

        // send an event to all registered handlers  for the given god.
        void dispatch(const GodId& god, const EventType& event) {
            // handlers_ is a shared resource which multiple threads can access here for respective god's
            // event handlers are copied to localHandlers object first so that access to handlers_ does not remain blocked while
            // executing event handler
            std::vector<EventHandler<EventType>> localHandlers;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (handlers_.count(god)) {
                    localHandlers = handlers_[god];
                }
            }
            // event is passed to all the handler's of God for processing 
            // For each registered handler, run it asynchronously and wait up to 100ms. This will ensure that task does not 
            // completely hold the resources 
            for (auto& handler : localHandlers) {
                auto fut = std::async(std::launch::async, handler, event);
                // Wait for the handler to complete, but only up to 100 milliseconds.
                if (fut.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout) {
                    std::cerr << "Handler for god \"" << god << "\" timed out." << std::endl;
                    // The handler will still be running in the background,
                    // but just logging timeout and moving on.
                }
                else {
                    // Currently event handlers are not returning anything but it can be use for exception catching
                    fut.get();
                }
            }
        }

    private:
        std::unordered_map<GodId, std::vector<EventHandler<EventType>>> handlers_;
        std::mutex mutex_;
    };

    // The main event handler class which encapsulates event queues for each god.
    // It creates a worker thread that continuously dispatches events in a round-robin fashion.
    template<typename EventType>
    class EldenTreeEventHandler {
    public:
        using GodId = std::string;

        EldenTreeEventHandler() : stop_flag_(false) {
            worker_thread_ = std::thread(&EldenTreeEventHandler::worker, this);
        }

        ~EldenTreeEventHandler() {
            stop_flag_.store(true);
            cv_.notify_all();
            if (worker_thread_.joinable())
                worker_thread_.join();
        }

        // Register an event handler for a given god.
        void registerHandler(const GodId& god, EventHandler<EventType> handler) {
            messenger_.registerHandler(god, handler);
        }

        // Push an event for a specific god.
        void pushEvent(const GodId& god, const EventType& event) {
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                god_event_queues_[god].push(event);
            }
            cv_.notify_all();
        }

        // (Optional) Synchronous processing in case you want manual control.
        void processEvents() {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            for (auto& pair : god_event_queues_) {
                const GodId& god = pair.first;
                auto& q = pair.second;
                while (!q.empty()) {
                    auto event = q.front();
                    q.pop();
                    lock.unlock();  // unlock while processing
                    messenger_.dispatch(god, event);
                    lock.lock();
                }
            }
        }

    private:
        // Worker thread that waits for events and executes them fairly.
        void worker() {
            while (!stop_flag_.load()) {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait_for(lock, std::chrono::milliseconds(1), [this] { return stop_flag_.load() || !allQueuesEmpty(); });
                bool processed = false;
                // Process one event per god in a round-robin fashion.
                for (auto& pair : god_event_queues_) {
                    const GodId& god = pair.first;
                    auto& q = pair.second;
                    if (!q.empty()) {
                        auto event = q.front();
                        q.pop();
                        lock.unlock();
                        messenger_.dispatch(god, event);
                        processed = true;
                        lock.lock();
                    }
                }
                if (!processed) {
                    lock.unlock();
                    std::this_thread::yield();
                }
            }
        }

        // Check if all god event queues are empty.
        bool allQueuesEmpty() {
            for (auto& pair : god_event_queues_) {
                if (!pair.second.empty())
                    return false;
            }
            return true;
        }

        std::unordered_map<GodId, std::queue<EventType>> god_event_queues_;
        std::mutex queue_mutex_;
        std::condition_variable cv_;
        std::atomic<bool> stop_flag_;
        std::thread worker_thread_;
        EventMessenger<EventType> messenger_;
    };

} // namespace elden_tree_ns

#endif // ELDEN_TREE_EVENT_HANDLER_H
  