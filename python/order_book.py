"""
Python prototype of the limit order book matching engine.
Mirrors the logic in the C++ version (Order.h / Trade.h / OrderBook.h/.cpp)
so the two can be fairly compared in a benchmark.
"""

import random
import time
from enum import Enum
from dataclasses import dataclass


class Side(Enum):
    BUY = 0
    SELL = 1


@dataclass
class Order:
    id: int
    side: Side
    price: float
    quantity: int


@dataclass
class Trade:
    buy_order_id: int
    sell_order_id: int
    price: float
    quantity: int


class OrderBook:
    def __init__(self):
        self.buy_orders = []   # list of Order
        self.sell_orders = []  # list of Order

    def add_order(self, order: Order):
        opposite = self.sell_orders if order.side == Side.BUY else self.buy_orders

        i = 0
        while i < len(opposite) and order.quantity > 0:
            resting_order = opposite[i]

            prices_cross = (
                (order.side == Side.BUY and order.price >= resting_order.price) or
                (order.side == Side.SELL and order.price <= resting_order.price)
            )

            if not prices_cross:
                i += 1
                continue

            trade_qty = min(order.quantity, resting_order.quantity)

            
            trade = Trade(
                buy_order_id=order.id if order.side == Side.BUY else resting_order.id,
                sell_order_id=order.id if order.side == Side.SELL else resting_order.id,
                price=resting_order.price,
                quantity=trade_qty,
            )
            _ = trade  # unused, mirrors the C++ (void)trade; line

            order.quantity -= trade_qty
            resting_order.quantity -= trade_qty

            if resting_order.quantity == 0:
                opposite.pop(i)  # fully filled, remove it
              
            else:
                i += 1  

        if order.quantity > 0:
            if order.side == Side.BUY:
                self.buy_orders.append(order)
            else:
                self.sell_orders.append(order)

    def print_book(self):
        print("\n--- Order Book ---")
        print(f"BUY orders: {len(self.buy_orders)}")
        print(f"SELL orders: {len(self.sell_orders)}")
        print("------------------\n")


def percentile(sorted_data, p):
    if not sorted_data:
        return 0.0
    idx = int(p * (len(sorted_data) - 1))
    return sorted_data[idx]


def main():
    NUM_ORDERS = 10000
    BASE_PRICE = 100.0
    PRICE_SPREAD = 5.0
    MIN_QTY = 1
    MAX_QTY = 50

    random.seed(42) 
    book = OrderBook()

    # Pre-generate all orders first, same as the C++ version,
    # so order generation isn't counted in the timing measurements.
    orders = []
    for i in range(1, NUM_ORDERS + 1):
        side = Side.BUY if random.randint(0, 1) == 0 else Side.SELL
        price = random.uniform(BASE_PRICE - PRICE_SPREAD, BASE_PRICE + PRICE_SPREAD)
        quantity = random.randint(MIN_QTY, MAX_QTY)
        orders.append(Order(i, side, price, quantity))

    # Benchmark: time EACH individual add_order() call.
    latencies_microseconds = []

    total_start = time.perf_counter()

    for order in orders:
        start = time.perf_counter()
        book.add_order(order)
        end = time.perf_counter()
        latencies_microseconds.append((end - start) * 1_000_000)  # convert to microseconds

    total_end = time.perf_counter()
    total_millis = (total_end - total_start) * 1000

    # ---------- Report results ----------
    latencies_microseconds.sort()

    mean = sum(latencies_microseconds) / len(latencies_microseconds)
    p50 = percentile(latencies_microseconds, 0.50)
    p95 = percentile(latencies_microseconds, 0.95)
    p99 = percentile(latencies_microseconds, 0.99)
    max_latency = latencies_microseconds[-1]

    throughput = NUM_ORDERS / (total_millis / 1000.0)  # orders per second

    print("===== Benchmark Results (Python) =====")
    print(f"Orders processed:   {NUM_ORDERS}")
    print(f"Total time:          {total_millis:.4f} ms")
    print(f"Throughput:          {throughput:.2f} orders/sec")
    print("------------------------------")
    print("Per-order latency (microseconds):")
    print(f"  mean: {mean:.4f}")
    print(f"  p50:  {p50:.4f}")
    print(f"  p95:  {p95:.4f}")
    print(f"  p99:  {p99:.4f}")
    print(f"  max:  {max_latency:.4f}")
    print("==============================")

    book.print_book()


if __name__ == "__main__":
    main()