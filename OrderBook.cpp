#include "OrderBook.h"
#include <iostream>


void OrderBook::removeEmptyKey(std::map<double, std::set<std::shared_ptr<OrderNode>, PtrComparator<std::shared_ptr<OrderNode>>>>& book, double price) {
    if(!book[price].empty()) return;
    if(!book.count(price)) return;

    book.erase(price);

    return;
}

OrderBook::OrderBook(std::string str) : ticker(str) {}

bool OrderBook::addBidOrder(double orderPrice, long long orderQuantity, long long id) {
    //first we go through the ask book to match existing orders
    //try match order with price <= bid order price
    std::deque<double> avilPriceLevels;
    for(auto& entry : askBook) //iterate by reference since we are working with unique_ptr
        if(!entry.second.empty()) // if the current price has sell order
            avilPriceLevels.push_back(entry.first);
        bool filled = false;

        while(!filled && !avilPriceLevels.empty()){
            //match from the lowest price
            double price = avilPriceLevels[0];
            std::unique_lock<std::mutex> ul(buymtx);
            avilPriceLevels.pop_front();
            while(askBook.count(price) && !askBook[price].empty() && !filled){
                auto askOrderForMatch = askBook[price].begin();
                if((*askOrderForMatch)->getQuantity() < orderQuantity){
                    orderQuantity -= (*askOrderForMatch)->getQuantity();
                    id2OrderPtr.erase((*askOrderForMatch)->getId());
                    askBook[price].erase(askOrderForMatch);
                    this->removeEmptyKey(askBook , price);
                }else{
                    //can be completed filled in this turn
                    if((*askOrderForMatch)->getQuantity() == orderQuantity){
                        askBook[price].erase(askOrderForMatch);
                        id2OrderPtr.erase((*askOrderForMatch)->getId());
                        this->removeEmptyKey(askBook , price);
                    }else{
                        (*askOrderForMatch)->setQuantity(-orderQuantity);
                    }
                    filled = true;
                    break;
                }
            }
            this->removeEmptyKey(askBook , price);
        }
        if(filled) return true;

        //if the order is not completed filled, we ask it to the book
        std::shared_ptr<OrderNode> newOrder(new OrderNode(buy , orderPrice , id, orderQuantity));
        bidBook[orderPrice].insert(newOrder);
        id2OrderPtr[id] = std::make_pair(orderPrice , newOrder);
        return true;
}

bool OrderBook::addAskOrder(double orderPrice, long long orderQuantity, long long id) {
    std::deque<double> avilPriceLevels;
    for(auto& entry : bidBook)
        if(!entry.second.empty())
            avilPriceLevels.push_back(entry.first);

        bool filled = false;
        while(!filled && !avilPriceLevels.empty()){
            //match from the highest bid price
            double price = avilPriceLevels.back();
            avilPriceLevels.pop_back();
            std::unique_lock<std::mutex> ul(sellmtx);
            while(bidBook.count(price) && !bidBook[price].empty() && !filled){
                auto bidOrderForMatch = bidBook[price].begin();
                if((*bidOrderForMatch)->getQuantity() < orderQuantity){
                    orderQuantity -= (*bidOrderForMatch)->getQuantity();
                    id2OrderPtr.erase((*bidOrderForMatch)->getId());
                    bidBook[price].erase(bidOrderForMatch);
                    this->removeEmptyKey(bidBook , price);
                }else{
                    //can be completed filled in this turn
                    if((*bidOrderForMatch)->getQuantity() == orderQuantity){
                        bidBook[price].erase(bidOrderForMatch);
                        id2OrderPtr.erase((*bidOrderForMatch)->getId());
                        this->removeEmptyKey(bidBook , price);
                    }else{
                        (*bidOrderForMatch)->setQuantity(-orderQuantity);
                    }
                    filled = true;
                    break;
                }
                this->removeEmptyKey(bidBook , price);
            }
        }
        if(filled) return true;

        //if the order is not completed filled, we ask it to the book
        std::shared_ptr<OrderNode> newOrder(new OrderNode(sell , orderPrice , id, orderQuantity));
        askBook[orderPrice].insert(newOrder);
        id2OrderPtr[id] = std::make_pair(orderPrice , newOrder);
        return true;
}

void OrderBook::showBook() {
    std::cout << "bid book size "<<bidBook.size() << std::endl;
    std::cout << "-----------------------" <<std::endl;
    std::cout << "ID\t|\tP\t|\t#\n";
    for(auto& entry : bidBook){
        for(auto& orderNode : bidBook[entry.first]){ //do not invoke the copy constructor or ownership transfer
            std::cout << orderNode->getId() << "\t|\t"<<orderNode->getPrice() <<"\t|\t"<<orderNode->getQuantity() << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "-----------------------\n\n\nask book size " <<askBook.size() << std::endl;


    std::cout << "ID\t|\tP\t|\t#\n";
    for(auto& entry : askBook){
        for(auto& orderNode : askBook[entry.first]){ //do not invoke the copy constructor or ownership transfer
            std::cout << orderNode->getId() << "\t|\t"<<orderNode->getPrice() <<"\t|\t"<<orderNode->getQuantity() << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "-----------------------" <<std::endl;
    std::cout << "#######################\n\n" << std::endl;
}

bool OrderBook::removeOrder(OrderType type, long long orderID) {
    if(!id2OrderPtr.count(orderID)) return false; //the order does not exist
    auto orderInfo = id2OrderPtr[orderID];
    if(type == sell){
        askBook[orderInfo.first].erase(orderInfo.second);
        removeEmptyKey(askBook , orderInfo.first);
    }else{
        bidBook[orderInfo.first].erase(orderInfo.second);
        removeEmptyKey(bidBook , orderInfo.first);
    }
    return true;
}

double OrderBook::getHighestBidOrder() {
    if(bidBook.empty()) return -1;
    auto highestEntry = bidBook.rbegin();
    return highestEntry->first;
}

double OrderBook::getLowestAskOrder() {
    if(askBook.empty()) return -1;
    auto lowestEntry = askBook.begin();
    return lowestEntry->first;
}

double OrderBook::getTotalBidVolumeAtPrice(double price) {
    if(!bidBook.count(price)) return 0;
    double totalVolume = 0;
    for(auto& entry : bidBook[price]) totalVolume += entry->orderPrice;
    return totalVolume;
}

double OrderBook::getTotalAskVolumeAtPrice(double price) {
    if(!askBook.count(price)) return 0;
    double totalVolume = 0;
    for(auto& entry : askBook[price]) totalVolume += entry->orderPrice;
    return totalVolume;
}

std::string OrderBook::getTicker() const {
    return this->ticker;
}

