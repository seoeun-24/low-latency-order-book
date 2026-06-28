#pragma once

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
