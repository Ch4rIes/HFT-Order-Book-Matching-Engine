#include "OrderNode.h"

OrderNode::OrderNode(OrderType ot, double op, long long id, long long quant) {
    time = std::chrono::system_clock::now().time_since_epoch().count(); // grab the current time in the system
    orderPrice = op;
    orderType = ot;
    orderID = id;
    quantity = quant;
}

void OrderNode::setQuantity(long long change) {
    this->quantity += change;
}

bool OrderNode::operator < (const OrderNode& other) const {
    return time < other.time;
}

bool OrderNode::operator > (const OrderNode& other) const {
    return time > other.time;
}

long long OrderNode::getQuantity() const {
    return this->quantity;
}

double OrderNode::getPrice() const {
    return this->orderPrice;
}

long long OrderNode::getId() const {
    return this->orderID;
}

bool OrderNode::operator == (const OrderNode& other) const {
    return time == other.time && orderPrice == other.orderPrice && orderType == other.orderType;
}
