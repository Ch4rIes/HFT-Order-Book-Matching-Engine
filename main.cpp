#include <iostream>
#include <unistd.h>
#include <set>
#include <map>
#include <string>
#include <memory>
#include "OrderBook.h"
#include "webserver.h"
#include <thread>

Exchange* exchange;

int main() {
    exchange = Exchange::getInstance();
    exchange->addTicker("APPL");
    exchange->addTicker("TSLA");
    exchange->addTicker("DRBK");
    exchange->addTicker("BX");
    exchange->addTicker("AMZN");
    WebServer server(8080);
    server.run();
}
