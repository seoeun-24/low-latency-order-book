#include <iostream>
#include <algorithm>
#include "OrderBook.h"
#include "Trade.h"
void OrderBook::addOrder(Order order)
{
   
    std::vector<Order> &opposite = (order.side == Side::BUY) ? sellOrders : buyOrders;

   
    for (size_t i = 0; i < opposite.size() && order.quantity > 0;)
    {
        Order &restingOrder = opposite[i];

        bool pricesCross =
            (order.side == Side::BUY && order.price >= restingOrder.price) ||
            (order.side == Side::SELL && order.price <= restingOrder.price);

        if (!pricesCross)
        {
            i++; // no match here, move to next resting order
            continue;
        }

      
        int tradeQty = std::min(order.quantity, restingOrder.quantity);

        Trade trade{
            (order.side == Side::BUY) ? order.id : restingOrder.id,
            (order.side == Side::SELL) ? order.id : restingOrder.id,
            restingOrder.price,
            tradeQty};

        // std::cout << "TRADE: buy#" << trade.buyOrderId
        //  << " <-> sell#" << trade.sellOrderId
        //   << " | qty=" << trade.quantity
        //    << " @ $" << trade.price << "\n";

        order.quantity -= tradeQty;
        restingOrder.quantity -= tradeQty;

        if (restingOrder.quantity == 0)
        {
            opposite.erase(opposite.begin() + i); 
        }
        else
        {
            i++;
        }
    }

    if (order.quantity > 0)
    {
        if (order.side == Side::BUY)
        {
            buyOrders.push_back(order);
        }
        else
        {
            sellOrders.push_back(order);
        }
    }
}

void OrderBook::printBook() const
{
    std::cout << "\n--- Order Book ---\n";
    std::cout << "BUY orders:\n";
    for (const auto &o : buyOrders)
    {
        std::cout << "  #" << o.id << " qty=" << o.quantity << " @ $" << o.price << "\n";
    }
    std::cout << "SELL orders:\n";
    for (const auto &o : sellOrders)
    {
        std::cout << "  #" << o.id << " qty=" << o.quantity << " @ $" << o.price << "\n";
    }
    std::cout << "------------------\n\n";
}
