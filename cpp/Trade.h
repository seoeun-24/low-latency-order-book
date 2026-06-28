#pragma once

// Created whenever a buy and sell order match.
struct Trade
{
    int buyOrderId;
    int sellOrderId;
    double price;
    int quantity;
};