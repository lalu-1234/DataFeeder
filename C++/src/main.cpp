#include "OptionChainCreater.hpp"
#include "TablePlotter.hpp"
#include <thread>
#include <chrono>

int main(){
    OptionChainCreater OCC(10, 10, "../Ticker.csv");
    thread t1(&OptionChainCreater::readTokenData, &OCC, "../tickers/", 50);

    this_thread::sleep_for(chrono::milliseconds(5000));
    TablePlotter TP;
    for(int i = 0; ; i++){
        TP.displayTable(OCC);
        this_thread::sleep_for(chrono::milliseconds(1000));
    }

    t1.join();
    return 0;
}
