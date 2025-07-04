#include "OptionChainCreater.hpp"

StrikeToTokenMapper::StrikeToTokenMapper(string path){
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

            if(option == "50") m_50_token = stoi(token);
            if(option == "UT") m_UT_token = stoi(token);
            
            M.insert({{stoi(strike), option}, stoi(token)});

        }catch(...){
            cerr << "Skipping invalid row: " << line << endl;
        }
    }

    Ticker.close();
}


int StrikeToTokenMapper::getToken(int strike, const string& option) const {
    auto i = M.find({strike, option});

    if(i != M.end()) 
        return i->second;
        
    return -1;
}


OptionChainCreater::OptionChainCreater(int UR, int LR, string path)
                        : m_UR(UR), m_LR(LR), StrikeToTokenMapper(path) {}


vector<double> OptionChainCreater::getLTP(string path){
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


pair<string, string> OptionChainCreater::getTimestamp(int unix_time){
    char buf1[80], buf2[80];
    time_t curr_time = time(nullptr);
    time_t exch_time = unix_time;
        
    struct tm *timeinfo1 = localtime(&curr_time);
    struct tm *timeinfo2 = localtime(&exch_time);

    strftime(buf1, sizeof(buf1), "%Y-%m-%d %H:%M:%S", timeinfo1);
    strftime(buf2, sizeof(buf2), "%Y-%m-%d %H:%M:%S", timeinfo2);
        
    return {buf1, buf2};
}


void OptionChainCreater::readTokenData(string path, int strike_diff){
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

    for(int i = 0; ; i++) {
        vector<double> row = getLTP(m_50_path);
        int exch_time = row[0] / 1000;
            
        auto [curr, exch] = getTimestamp(exch_time);
        int ATM = getATM(row[1] / 100);

        if(!Ok) {
            for(int i = -m_UR; i <= m_LR; i++)
                strikes.push_back(ATM + (i * strike_diff));

            Ok = 1;
        } else {
            
            int diff = ATM - last_ATM;
            for(int k = min(0, diff); k < max(0, diff); k += strike_diff) {
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

        function<double(double)> round2Digit = [&](double x){
            return (round(x * 100.0) / 100.0);
        };

        vector<OptionChainFormat> new_oc;
        for(auto k : strikes) {
            int token1 = getToken(k, "CE");
            string path1 = path + to_string(token1);
            vector<double> row1 = getLTP(path1);
            
            int token2 = getToken(k, "PE");
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
            
            oc_row.strike = k;
            
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

vector<OptionChainFormat> OptionChainCreater::getOptionChain(){
    lock_guard<mutex> lock(mtx);
    return m_OC;
}
