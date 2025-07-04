#pragma once
#include <bits/stdc++.h>
using namespace std;

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

    StrikeToTokenMapper(string path);
    
    int getToken(int strike, const string& option) const;
};

class OptionChainCreater : public StrikeToTokenMapper {
private:
    int m_UR, m_LR;
    mutable mutex mtx;
    
    vector<OptionChainFormat> m_OC;

public:
    OptionChainCreater(int UR, int LR, string path);

    vector<double> getLTP(string path);
    
    pair<string, string> getTimestamp(int unix_time);
    
    void readTokenData(string path, int strike_diff);
    
    vector<OptionChainFormat> getOptionChain();
    
    friend class TablePlotter;
};
