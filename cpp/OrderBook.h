#pragma once

#include <vector>
#include "Order.h"

class OrderBook
{
public:
    void addOrder(Order order);
    void printBook() const;

private:
    std::vector<Order> buyOrders;
    std::vector<Order> sellOrders;
};