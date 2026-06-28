#include <iostream>
#include <vector>
#include <map>
#include <random>
#include <chrono>
#include <algorithm>

// ---------- Order ----------
enum class Side
{
    BUY,
    SELL
};

struct Order
{
    int id;
    Side side;
    double price;
    int quantity;
};

// ---------- Trade ----------
struct Trade
{
    int buyOrderId;
    int sellOrderId;
    double price;
    int quantity;
};

// ---------- OrderBook (map-based) ----------
// Buy side: highest price = best, so we want to iterate price descending.
// Sell side: lowest price = best, so we want to iterate price ascending.
// std::map keeps keys sorted ascending by default, so:
//   - sellLevels: iterate forward (begin() = lowest price = best)
//   - buyLevels:  iterate backward (rbegin() = highest price = best)
class OrderBookMap
{
public:
    void addOrder(Order order);
    void printBook() const;

private:
    std::map<double, std::vector<Order>> buyLevels;
    std::map<double, std::vector<Order>> sellLevels;
};

void OrderBookMap::addOrder(Order order)
{
    if (order.side == Side::BUY)
    {
        // Match against sell levels, starting from the lowest price (best for buyer).
        auto it = sellLevels.begin();
        while (it != sellLevels.end() && order.quantity > 0)
        {
            double levelPrice = it->first;
            if (order.price < levelPrice)
                break; // no more crossable levels

            std::vector<Order> &ordersAtLevel = it->second;
            size_t i = 0;
            while (i < ordersAtLevel.size() && order.quantity > 0)
            {
                Order &restingOrder = ordersAtLevel[i];
                int tradeQty = std::min(order.quantity, restingOrder.quantity);

                Trade trade{order.id, restingOrder.id, levelPrice, tradeQty};
                (void)trade;

                order.quantity -= tradeQty;
                restingOrder.quantity -= tradeQty;

                if (restingOrder.quantity == 0)
                {
                    ordersAtLevel.erase(ordersAtLevel.begin() + i);
                }
                else
                {
                    i++;
                }
            }

            if (ordersAtLevel.empty())
            {
                it = sellLevels.erase(it); // remove empty price level, advances iterator
            }
            else
            {
                ++it;
            }
        }

        if (order.quantity > 0)
        {
            buyLevels[order.price].push_back(order);
        }
    }
    else
    {
        // SELL: match against buy levels, starting from the highest price (best for seller).
        auto it = buyLevels.rbegin();
        while (it != buyLevels.rend() && order.quantity > 0)
        {
            double levelPrice = it->first;
            if (order.price > levelPrice)
                break;

            std::vector<Order> &ordersAtLevel = it->second;
            size_t i = 0;
            while (i < ordersAtLevel.size() && order.quantity > 0)
            {
                Order &restingOrder = ordersAtLevel[i];
                int tradeQty = std::min(order.quantity, restingOrder.quantity);

                Trade trade{restingOrder.id, order.id, levelPrice, tradeQty};
                (void)trade;

                order.quantity -= tradeQty;
                restingOrder.quantity -= tradeQty;

                if (restingOrder.quantity == 0)
                {
                    ordersAtLevel.erase(ordersAtLevel.begin() + i);
                }
                else
                {
                    i++;
                }
            }

            if (ordersAtLevel.empty())
            {
                // erase via reverse_iterator requires converting to a forward iterator
                it = std::map<double, std::vector<Order>>::reverse_iterator(
                    buyLevels.erase(std::next(it).base()));
            }
            else
            {
                ++it;
            }
        }

        if (order.quantity > 0)
        {
            sellLevels[order.price].push_back(order);
        }
    }
}

void OrderBookMap::printBook() const
{
    int buyCount = 0, sellCount = 0;
    for (const auto &[price, orders] : buyLevels)
        buyCount += orders.size();
    for (const auto &[price, orders] : sellLevels)
        sellCount += orders.size();

    std::cout << "\n--- Order Book (map-based) ---\n";
    std::cout << "BUY orders: " << buyCount << "\n";
    std::cout << "SELL orders: " << sellCount << "\n";
    std::cout << "------------------\n\n";
}

// ---------- Benchmark helpers ----------
double percentile(const std::vector<double> &sortedData, double p)
{
    if (sortedData.empty())
        return 0.0;
    size_t idx = static_cast<size_t>(p * (sortedData.size() - 1));
    return sortedData[idx];
}

int main()
{
    const int NUM_ORDERS = 10000;
    const double BASE_PRICE = 100.0;
    const double PRICE_SPREAD = 5.0;
    const int MIN_QTY = 1;
    const int MAX_QTY = 50;

    OrderBookMap book;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> priceDist(BASE_PRICE - PRICE_SPREAD,
                                                     BASE_PRICE + PRICE_SPREAD);
    std::uniform_int_distribution<int> qtyDist(MIN_QTY, MAX_QTY);
    std::uniform_int_distribution<int> sideDist(0, 1);

    std::vector<Order> orders;
    orders.reserve(NUM_ORDERS);
    for (int i = 1; i <= NUM_ORDERS; i++)
    {
        Side side = (sideDist(gen) == 0) ? Side::BUY : Side::SELL;
        double price = priceDist(gen);
        int quantity = qtyDist(gen);
        orders.push_back({i, side, price, quantity});
    }

    std::vector<double> latenciesMicroseconds;
    latenciesMicroseconds.reserve(NUM_ORDERS);

    auto totalStart = std::chrono::high_resolution_clock::now();

    for (const auto &order : orders)
    {
        auto start = std::chrono::high_resolution_clock::now();
        book.addOrder(order);
        auto end = std::chrono::high_resolution_clock::now();
        latenciesMicroseconds.push_back(
            std::chrono::duration<double, std::micro>(end - start).count());
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    double totalMillis = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    std::sort(latenciesMicroseconds.begin(), latenciesMicroseconds.end());

    double sum = 0;
    for (double l : latenciesMicroseconds)
        sum += l;
    double mean = sum / latenciesMicroseconds.size();

    double p50 = percentile(latenciesMicroseconds, 0.50);
    double p95 = percentile(latenciesMicroseconds, 0.95);
    double p99 = percentile(latenciesMicroseconds, 0.99);
    double maxLatency = latenciesMicroseconds.back();
    double throughput = NUM_ORDERS / (totalMillis / 1000.0);

    std::cout << "===== Benchmark Results (std::map version) =====\n";
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