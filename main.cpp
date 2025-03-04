#include "EldenTreeEventHandler.h"
#include <iostream>
#include <chrono>
#include <thread>

// A simple event structure
struct Event {
    int id;
    std::string message;
};

int main() {
    // Create an instance of the event handler for Event types.
    elden_tree_ns::EldenTreeEventHandler<Event> handler;

    // Register handlers for different gods.
    handler.registerHandler("God_1", [](const Event& e) {
        // Process God_1' event (could be extended with more complex logic)1
        std::cout << "[God_1] Processing event " << e.id << ": " << e.message << std::endl;
        volatile int u = 0;

        for(volatile int i=0; i<100000000; i++)
        {
            u++;
        }
        });
    handler.registerHandler("God_2", [](const Event& e) {
        std::cout << "[God_2] Processing event " << e.id << ": " << e.message << std::endl;
        });

    // Push some initial events.
    for (int i = 0; i < 10; ++i) {
        handler.pushEvent("God_1", Event{ i, "Event from God 1 is processessing" });
        handler.pushEvent("God_2", Event{ i, "Event from God 2 is processessing" });
    }

    // Allow some time for asynchronous processing.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Benchmark: measure the performance for processing a large number of events.
    const int numEvents = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numEvents; ++i) {
        handler.pushEvent("God_1", Event{ i, "Benchmark event" });
    }
    // Wait a little to ensure events are processed.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Processed " << numEvents << " benchmark events in "
        << elapsed.count() << " seconds." << std::endl;

    return 0;
}
