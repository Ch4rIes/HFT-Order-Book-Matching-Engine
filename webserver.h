#ifndef ORDERMATCHINGENGINE_WEBSERVER_H
#define ORDERMATCHINGENGINE_WEBSERVER_H


#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <map>
#include "OrderBook.h"
#include <unordered_map>
#include <string>
#include <thread>
#include "OrderBookTaskQueue.h"
using boost::asio::ip::tcp;


//forward declearation
class ClientSession;
class OrderBookTaskQueue;
class WebServer;

struct RequestInfo: public std::enable_shared_from_this<RequestInfo>{
    std::map<std::string , std::string> body;
    std::shared_ptr<ClientSession> session;
};

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];

    void read_data();
    std::vector<std::string> split(std::string str, char split);
    std::shared_ptr<RequestInfo> parse_request(std::string request);
    void process_data(std::string request);

    ClientSession(tcp::socket sock);
    void start();
    void write_data(const std::string &response);
};


class OrderBookTaskQueue{
    std::shared_ptr<OrderBook> orderBook;
    std::deque<std::shared_ptr<RequestInfo>> taskQueue;
    std::condition_variable cv;
public:
    std::string ticker;
    OrderBookTaskQueue(std::string ticker){
        orderBook = std::make_shared<OrderBook>(ticker);
        this->ticker = ticker;
    }

    void operate(){
        std::cout << orderBook->getTicker() << " starts operating\n";
        while(true){
            std::mutex m;
            std::unique_lock<std::mutex> ul(m);
            while(taskQueue.empty()){
                cv.wait(ul);
            }
            std::shared_ptr<RequestInfo> task = taskQueue.front();
            taskQueue.pop_front();
            std::string responseMsg;
            //now do case work for different request types
            std::string requestType = task->body["REQUEST_TYPE"];
            if(requestType == "getTotalAskVolumnAtPrice"){
                responseMsg = std::to_string(orderBook->getTotalAskVolumeAtPrice(10));
            }else if(requestType == "AddOrder"){

            }
            //TODO: add handler for all types of requests

            task->session->write_data(responseMsg);
        }
        return;
    }

    void addTasks(std::shared_ptr<RequestInfo> task){
        taskQueue.push_back(task);
        cv.notify_all();
        return;
    }
};


//singleton design
class Exchange{
private:
    std::unordered_map<std::string , std::shared_ptr<OrderBookTaskQueue>> exchange;
    std::unordered_map<std::string , std::thread> ticker2Threads;
    static Exchange* instance;
    Exchange(){};

public:

    Exchange(Exchange &other) = delete;
    void operator=(const Exchange& other) = delete;

    static Exchange* getInstance(){
        if(instance != nullptr){
            return instance;
        }
        instance = new Exchange();
        return instance;
    }

    void addTicker(std::string ticker){
        if(exchange.count(ticker))
            return;

        std::string name = ticker;
        exchange[ticker] = std::make_shared<OrderBookTaskQueue>(name);

        // Start a thread for the ticker's OrderBookTaskQueue's operate method
        ticker2Threads[ticker] = std::thread(&OrderBookTaskQueue::operate, exchange[ticker].get());
    }

    void processRequest(std::shared_ptr<RequestInfo> req){
        if(!exchange.count(req->body["Ticker"]))
            throw std::logic_error("ticker has not been created");
        auto taskQueue = exchange[req->body["Ticker"]];
        taskQueue->addTasks(req);

    }

    void delTicker(std::string ticker){
        if(!exchange.count(ticker))
            return;
        exchange.erase(ticker);
    }

    std::shared_ptr<OrderBookTaskQueue> getOrderBookTaskQueuePtr(std::string ticker){
        if(!exchange.count(ticker)) return nullptr;
        return exchange[ticker];
    }
};

Exchange* Exchange::instance = nullptr;


void ClientSession::read_data() {
    auto self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            process_data(std::string(data_, length));
        }
    });
}

std::vector<std::string> ClientSession::split(std::string str , char split){
    std::vector<std::string> result;
    int end = str.find(split);
    while(end != -1){
        result.push_back(str.substr(0 , end));
        str.erase(str.begin() , str.begin() + end + 1);
        end = str.find(split);
    }
    result.push_back(str);
    return result;
}
std::shared_ptr<RequestInfo> ClientSession::parse_request(std::string request){
    auto splitted = split(request , '|'); //we separate key/val by |
    std::shared_ptr<RequestInfo> info = std::make_shared<RequestInfo>();
    for(int i = 0 ; i < splitted.size() ; i+=2){
        info->body[splitted[i]] = splitted[i + 1];
    }
    return info;
}
void ClientSession::process_data(std::string request){
    std::shared_ptr<RequestInfo> info = parse_request(request);
    info->session = shared_from_this();
    Exchange* exchange = Exchange::getInstance();
    exchange->processRequest(info);
    //write_data("processed");
}
void ClientSession::write_data(const std::string &response) {
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(response),
                             [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
            read_data();  // Ready to receive the next data
        }
    });
}
ClientSession::ClientSession(tcp::socket sock) : socket_(std::move(sock)) {}

void ClientSession::start() {
    read_data();
}


class WebServer {
private:
    boost::asio::io_service io_service_;
    tcp::acceptor acceptor_;

    void start_accept() {
        auto new_socket = std::make_shared<tcp::socket>(io_service_);
        acceptor_.async_accept(*new_socket, [this, new_socket](boost::system::error_code ec) {
            if (!ec) {
                auto session = std::make_shared<ClientSession>(std::move(*new_socket));
                session->start();
            }
            start_accept();  // Start accepting the next connection.
        });
    }

public:
    WebServer(int port)
    : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

    void run() {
        io_service_.run();
    }
};




#endif //ORDERMATCHINGENGINE_WEBSERVER_H