#include <tabulate/asciidoc_exporter.hpp>
#include <bits/stdc++.h>
#include <unistd.h>
#include <typeinfo>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <thread>
#include <deque>
#include <ctime>
#include <cmath>
#include <mutex>

using namespace tabulate;
using namespace std;

#define ss second
#define ff first

struct OptionChainFormat{
    double ltp1, change1, volume1, oi1, bsr1;
    double ltp2, change2, volume2, oi2, bsr2;
    int strike;
};

class StrikeToTokenMapper{
private:
    map<pair<int, string>, int> M;

public:
    int m_50_token {}; 
    int m_UT_token {};

    StrikeToTokenMapper(string path){
        ifstream Ticker(path);
        if(!Ticker.is_open()){
            cerr << "Error opening Ticker file: " << path << endl;
            return;
        }

        string line;
        while(getline(Ticker, line)){
            stringstream ss(line);
            string strike, option, token, expiry;

            try{
                getline(ss, strike, ',');
                getline(ss, option, ',');
                getline(ss, token, ',');
                getline(ss, expiry, ',');

                if(option == "50")  m_50_token = stoi(token);
                if(option == "UT")  m_UT_token = stoi(token);

                M.insert({{stoi(strike), option}, stoi(token)});
            }catch(...){
                cerr << "Skipping invalid row: " << line << endl;
            }
        }

        Ticker.close();
    }

    int getToken(int strike, const string& option) const {
        auto i = M.find({strike, option});

        if(i != M.end()) 
            return i->second;
        
        return -1;
    }
};

class OptionChainCreater : public StrikeToTokenMapper{
private :
    int m_UR {};
    int m_LR {};
    mutex mtx;

protected :
    vector<OptionChainFormat> m_OC;

public :

    OptionChainCreater(int UR, int LR, string path) 
        : m_UR { UR }, m_LR { LR }, StrikeToTokenMapper(path){ }
    
    friend class TablePlotter;

    vector<double> getLTP(string path){
        ifstream file(path, ios::binary);

        const size_t count = 7;
        vector<double> row(count);

        if(!file.is_open()){
            cerr << "Failed to open file: " << endl;
            return row;
        }

        file.read(reinterpret_cast<char*>(row.data()), count * sizeof(double));
        file.seekg(0);

        return row;
    }

    pair<string, string> getTimestamp(int unix_time){
        char buf1[80], buf2[80];
        time_t curr_time = time(nullptr);
        time_t exch_time = unix_time;
        
        struct tm *timeinfo1 = localtime(&curr_time);
        struct tm *timeinfo2 = localtime(&exch_time);

        strftime(buf1, sizeof(buf1), "%Y-%m-%d %H:%M:%S", timeinfo1);
        strftime(buf2, sizeof(buf2), "%Y-%m-%d %H:%M:%S", timeinfo2);
        
        return {buf1, buf2};
    }
    
    void readTokenData(string path, int strike_diff){
        string m_50_path = path + to_string(m_50_token);
        string m_UT_path = path + to_string(m_UT_token);

        int last_ATM, Ok;
        last_ATM = Ok = 0; 
        deque<int> strikes;

        function<double(int)> getATM = [&](double spot){
            double diff1 = fmod(spot, 50);
            double diff2 = 50 - diff1;

            return ((diff1 < diff2) ? (spot - diff1) : (spot + diff2));
        };

        for(int i = 0; i < 10; i++){
            vector<double> row = getLTP(m_50_path);
            int exch_time = row[0] / 1000;
            
            auto [curr, exch] = getTimestamp(exch_time);
            int ATM = getATM(row[1] / 100);

            if(!Ok){    
                for(int i = -m_UR; i <= m_LR; i++)
                strikes.push_back(ATM + (i * strike_diff)); 
                
                Ok = 1;
            }else{
                int diff = ATM - last_ATM;
                if(diff > 0 || diff < 0){
                    for(int k = min(0, diff); k < max(0, diff); k += strike_diff){
                        if(k < 0){
                            int new_strike = strikes.front() - strike_diff;
                            strikes.push_front(new_strike);
                            strikes.pop_back();
                        }else{
                            int new_strike = strikes.back() + strike_diff;
                            strikes.push_back(new_strike);
                            strikes.pop_front();
                        }
                    }
                }
            }

            function<double(double)> round2Digit = [&](double x){
                return (round(x * 100.0) / 100.0);
            };

            vector<OptionChainFormat> new_oc;
            for(auto k = strikes.begin(); k != strikes.end(); ++k){
                
                int token1 = getToken(*k, "CE");
                string path1 = path + to_string(token1);
                vector<double> row1 = getLTP(path1);
                
                int token2 = getToken(*k, "PE");
                string path2 = path + to_string(token2);
                vector<double> row2 = getLTP(path2);
                
                row1[1] /= 100;
                row2[1] /= 100;
                row1[2] /= 100;
                row2[2] /= 100;
                
                OptionChainFormat oc_row;
                
                oc_row.ltp1 = round2Digit(row1[1]);
                oc_row.ltp2 = round2Digit(row2[1]);
                
                oc_row.change1 = round2Digit((row1[1] - row1[2]) / row1[2]);
                oc_row.change2 = round2Digit((row2[1] - row2[2]) / row2[2]);
                
                oc_row.volume1 = round2Digit(row1[3] / 100000.0);
                oc_row.volume2 = round2Digit(row2[3] / 100000.0); 
                
                oc_row.oi1 = round2Digit(row1[4] / 100000.0);
                oc_row.oi2 = round2Digit(row2[4] / 100000.0);
                
                oc_row.bsr1 = round2Digit(row1[5] / (double) row1[6]);
                oc_row.bsr2 = round2Digit(row2[5] / (double) row2[6]);
                
                oc_row.strike = *k;
                
                new_oc.push_back(oc_row);
            }

            {
                lock_guard<mutex> lock(mtx);
                m_OC = move(new_oc);
            }
            
            last_ATM = ATM;
            this_thread::sleep_for(chrono::milliseconds(1000));
        }   
    }

    vector<OptionChainFormat> getOptionChain(){
        lock_guard<mutex> lock(mtx);
        return m_OC;
    }
};

class TablePlotter{
public :
    string toString(double x){
        stringstream ss;    
        ss << fixed << setprecision(2) << x;
        return ss.str();
    };
            
    void prepareTable(vector<OptionChainFormat>& v){

        Table OptionChain;
        OptionChain.add_row({"BSR1", "OI1", "VOLUME1", "% CHANGE1", "LTP1", "STRIKE", "LTP2", "% CHANGE2", "VOLUME2", "OI2", "BSR2"});
        
        for(auto k : v){
            OptionChain.add_row({
                toString(k.bsr1), toString(k.oi1), toString(k.volume1), toString(k.change1), 
                toString(k.ltp1), to_string(k.strike), toString(k.ltp2),
                toString(k.change2), toString(k.volume2), toString(k.oi2), toString(k.bsr2)
            });   
        }
        
        int OCsize = OptionChain.size();
        OptionChain.format()
            .corner("+")
            .border_top("-")
            .border_bottom("-")
            .border_left("|")
            .border_right("|");

        for(size_t i = 2; i < OCsize - 1; i++){
            OptionChain[i].format()
                .hide_border_top()
                .hide_border_bottom();
        }

        OptionChain[OCsize - 1].format()
            .hide_border_top()
            .border_bottom("-");

        OptionChain[0].format()
            .font_style({FontStyle::bold}); 
        
        for(size_t i = 0; i < OptionChain.size(); i++){
            OptionChain[i].format().
            font_align(FontAlign::center);
        }

        cout << OptionChain << endl;
    }

    void displayTable(OptionChainCreater& f){
        prepareTable(f.m_OC);
    }

    void displayTable(vector<OptionChainFormat>& v){
        prepareTable(v);
    }
};

int main(){
    
    int UR, LR;
    UR = LR = 5;
    OptionChainCreater OCC(UR, LR, "../Ticker.csv");
    
    thread t1(&OptionChainCreater::readTokenData, &OCC, "../tickers/", 50);

    this_thread::sleep_for(chrono::milliseconds(5000));

    TablePlotter TP;
    for(int i = 0; i < 10; i++){
        vector<OptionChainFormat> oc = OCC.getOptionChain();
        TP.displayTable(oc);
        this_thread::sleep_for(chrono::milliseconds(1000));
    }

    t1.join();

    return 0;
}