//
// Created by Charles_Z on 2023-09-12.
//

#ifndef ORDERMATCHINGENGINE_ORDERBOOK_H
#define ORDERMATCHINGENGINE_ORDERBOOK_H

#endif //ORDERMATCHINGENGINE_ORDERBOOK_H
#pragma once

#include "OrderNode.h"
#include <set>
#include <unordered_map>
#include <map>
#include <mutex>
#include <vector>
#include <deque>

class OrderBook {
private:
    std::map<double, std::set<std::shared_ptr<OrderNode>, PtrComparator<std::shared_ptr<OrderNode>>>> bidBook, askBook;
    std::unordered_map<long long, std::pair<double, std::shared_ptr<OrderNode>>> id2OrderPtr;
    std::string ticker;
    std::mutex mtx;
    std::mutex buymtx, sellmtx;

    void removeEmptyKey(std::map<double, std::set<std::shared_ptr<OrderNode>, PtrComparator<std::shared_ptr<OrderNode>>>>& book, double price);

public:
    OrderBook(std::string str);

    bool addBidOrder(double orderPrice, long long orderQuantity, long long id);
    bool addAskOrder(double orderPrice, long long orderQuantity, long long id);
    void showBook();
    bool removeOrder(OrderType type, long long orderID);
    double getHighestBidOrder();
    double getLowestAskOrder();
    double getTotalBidVolumeAtPrice(double price);
    double getTotalAskVolumeAtPrice(double price);
    std::string getTicker() const;
};
