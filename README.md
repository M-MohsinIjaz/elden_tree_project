# elden_tree_project

# Elden Tree Event Handler Library
Elden Tree is a high-performance, header-only C++ library for asynchronous and fair event handling. Designed for a mythological world where multiple gods send events, the library efficiently processes events using modern C++ features such as templates, threads, condition variables, and asynchronous tasks with timeouts.

# Features
# Templated Design:
Works with any event type, allowing you to define custom event structures.

# Multiple Handlers per God:
Supports registration of multiple event handlers for each unique source (god).

# Asynchronous Dispatch:
Uses a dedicated worker thread to continuously process events from per-god queues.

# Timeout Enforcement:
Each event handler is executed asynchronously with a 100ms timeout to prevent a slow handler from blocking event processing.

# Fair Round-Robin Processing:
Events from each god are processed in a round-robin manner to ensure no single source starves others.

# Thread-Safe:
Utilizes mutexes, condition variables, and atomic operations to guarantee safe concurrent access.
