#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include "OrderBook.h"

// Computes the p-th percentile (0.0 to 1.0) of a *sorted* vector of doubles.
double percentile(const std::vector<double> &sortedData, double p)
{
    if (sortedData.empty())
        return 0.0;
    size_t idx = static_cast<size_t>(p * (sortedData.size() - 1));
    return sortedData[idx];
}

int main()
{
    const int NUM_ORDERS = 10000; // how many orders to benchmark
    const double BASE_PRICE = 100.0;
    const double PRICE_SPREAD = 5.0;
    const int MIN_QTY = 1;
    const int MAX_QTY = 50;

    OrderBook book;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> priceDist(BASE_PRICE - PRICE_SPREAD,
                                                     BASE_PRICE + PRICE_SPREAD);
    std::uniform_int_distribution<int> qtyDist(MIN_QTY, MAX_QTY);
    std::uniform_int_distribution<int> sideDist(0, 1);

    // Pre-generate all orders first, so order generation itself
   
    std::vector<Order> orders;
    orders.reserve(NUM_ORDERS);
    for (int i = 1; i <= NUM_ORDERS; i++)
    {
        Side side = (sideDist(gen) == 0) ? Side::BUY : Side::SELL;
        double price = priceDist(gen);
        int quantity = qtyDist(gen);
        orders.push_back({i, side, price, quantity});
    }

    // Now benchmark: time EACH individual addOrder() call.

    
    std::vector<double> latenciesMicroseconds;
    latenciesMicroseconds.reserve(NUM_ORDERS);

    auto totalStart = std::chrono::high_resolution_clock::now();

    for (const auto &order : orders)
    {
        auto start = std::chrono::high_resolution_clock::now();
        book.addOrder(order);
        auto end = std::chrono::high_resolution_clock::now();

        double micros = std::chrono::duration<double, std::micro>(end - start).count();
        latenciesMicroseconds.push_back(micros);
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    double totalMillis = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    // ---------- Report results ----------
    std::sort(latenciesMicroseconds.begin(), latenciesMicroseconds.end());

    double sum = 0;
    for (double l : latenciesMicroseconds)
        sum += l;
    double mean = sum / latenciesMicroseconds.size();

    double p50 = percentile(latenciesMicroseconds, 0.50);
    double p95 = percentile(latenciesMicroseconds, 0.95);
    double p99 = percentile(latenciesMicroseconds, 0.99);
    double maxLatency = latenciesMicroseconds.back();

    double throughput = NUM_ORDERS / (totalMillis / 1000.0); // orders per second

    std::cout << "===== Benchmark Results =====\n";
    std::cout << "Orders processed:   " << NUM_ORDERS << "\n";
    std::cout << "Total time:          " << totalMillis << " ms\n";
    std::cout << "Throughput:          " << throughput << " orders/sec\n";
    std::cout << "------------------------------\n";
    std::cout << "Per-order latency (microseconds):\n";
    std::cout << "  mean: " << mean << "\n";
    std::cout << "  p50:  " << p50 << "\n";
    std::cout << "  p95:  " << p95 << "\n";
    std::cout << "  p99:  " << p99 << "\n";
    std::cout << "  max:  " << maxLatency << "\n";
    std::cout << "==============================\n";

    book.printBook();

    return 0;
}
