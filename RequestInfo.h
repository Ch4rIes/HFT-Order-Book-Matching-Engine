//
// Created by Charles_Z on 2023-09-19.
//

#ifndef ORDERMATCHINGENGINE_REQUESTINFO_H
#define ORDERMATCHINGENGINE_REQUESTINFO_H


#include <string>
#include <map>
#include <memory>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;

class ClientSession;



struct RequestInfo{
    std::map<std::string , std::string> body;
    ClientSession* session;
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
    info->session = this;
    Exchange* exchange = Exchange::getInstance();
    exchange->processRequest(info);
    std::cout << "process" << std::endl;
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


#endif //ORDERMATCHINGENGINE_REQUESTINFO_H
