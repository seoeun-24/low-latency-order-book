# Low-Latency Limit Order Book

A limit order book and matching engine implemented in Python and C++ to study low-latency trading system performance.

This project compares:
- Python vs C++ performance
- std::vector vs std::map order storage
- Throughput and latency tradeoffs
- Cache locality effects in systems programming

The project began as a Python prototype and was later ported to C++ for benchmarking and performance analysis.

Includes:
- Matching engine implementation
- Benchmarking framework
- Research-style paper
