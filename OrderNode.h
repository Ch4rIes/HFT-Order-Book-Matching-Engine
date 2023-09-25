//
// Created by Charles_Z on 2023-09-12.
//

#ifndef ORDERMATCHINGENGINE_ORDERNODE_H
#define ORDERMATCHINGENGINE_ORDERNODE_H

#endif //ORDERMATCHINGENGINE_ORDERNODE_H
#include <chrono>

enum OrderType {
    buy,
    sell
};

class OrderNode {
public:
    OrderNode(OrderType ot, double op, long long id, long long quant);

    void setQuantity(long long change);
    bool operator < (const OrderNode& other) const;
    bool operator > (const OrderNode& other) const;
    long long getQuantity() const;
    double getPrice() const;
    long long getId() const;
    bool operator == (const OrderNode& other) const;

private:
    OrderType orderType;
    double orderPrice;
    long long time;
    long long orderID;
    long long quantity;

    friend class OrderBook;
};

template<typename T>
struct PtrComparator {
    bool operator()(const T& lhs, const T& rhs) const{
        return *lhs < *rhs;
    };
};
