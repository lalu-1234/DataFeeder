#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <deque>

using namespace std;

#define ss second
#define ff first

int main(){
    ifstream Ticker("Ticker.csv");
    if(!Ticker.is_open()){
        cerr << "Error opening Ticker!" << endl;
        return 1;
    }

    map<pair<int, string>, int>  M;
    
    string line;
    while(getline(Ticker, line)){
        stringstream ss(line);
        string strike, option, token, expiry;

        try{
            getline(ss, strike, ',');
            getline(ss, option, ',');
            getline(ss, token, ',');
            getline(ss, expiry, ',');

            M[{stoi(strike), option}] = stoi(token);
        }catch(...){
            cerr << "Skipping invalid row: " << line << endl;
        }
    }

    auto getLTP = [&](string path) -> pair<int, double>{
        ifstream file(path, ios::binary);
        if(!file.is_open()){
            cerr << "Failed to open file: " << endl;
            return {1, 0.0};
        }

        const size_t count = 2;
        vector<double> values(count);

        file.read(reinterpret_cast<char*>(values.data()), count * sizeof(double));
        file.seekg(0);
        
        values[0] /= 1000;

        cout << fixed << setprecision(2);
        values[1] /= (double) 100;

        return {values[0], values[1]};
    };

    Ticker.close();

    mt19937 gen(42);
    uniform_real_distribution<> dis(25500.0, 25525.0);

    int upper_range, lower_range;
    upper_range = lower_range = 10;

    int Ok = 0;
    deque<float> strikes;
    float last_atm = 0.0;
    for(int i = 0; i < 10; i++){
        float spot = dis(gen);

        float diff1 = fmod(spot, 50);
        float diff2 = 50 - diff1;        
        float atm = (diff1 < diff2) ? (spot - diff1) : (spot + diff2);

        if(!Ok){    
            for(int i = -upper_range; i <= lower_range; i++)
                strikes.push_back(atm + (i * 50)); 
    
            Ok = 1;
        }else{
            int diff = atm - last_atm;
            if(diff > 0 || diff < 0){
                for(int i = min(0, diff); i < max(0, diff); i += 50){
                    if(i < 0){
                        int new_strike = strikes.front() - 50;
                        strikes.push_front(new_strike);
                        strikes.pop_back();
                    }else{
                        int new_strike = strikes.back() + 50;
                        strikes.push_back(new_strike);
                        strikes.pop_front();
                    }
                }
            }
        }

        for(auto it = strikes.begin(); it != strikes.end(); ++it){
            int token1 = M[{*it, "CE"}];
            string path1 = "./tickers/" + to_string(token1);
            auto ltp1 = getLTP(path1);

            int token2 = M[{*it, "PE"}];
            string path2 = "./tickers/" + to_string(token2);
            auto ltp2 = getLTP(path2);
            
            cout << ltp1.ss << " ";
            cout << *it << " ";
            cout << ltp2.ss << endl;
        }

        last_atm = atm;

        if(i == 1)
            break;

        cout << endl;
    }
    
    return 0;
}