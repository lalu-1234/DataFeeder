#include "TablePlotter.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace std;

string TablePlotter::toString(double x){
    stringstream ss;
    ss << fixed << setprecision(2) << x;
    return ss.str();
}

void TablePlotter::prepareTable(vector<OptionChainFormat>& v){
    Table OptionChain;
    OptionChain.add_row({"BSR1", "OI1", "VOLUME1", "% CHANGE1", "LTP1", "STRIKE", "LTP2", "% CHANGE2", "VOLUME2", "OI2", "BSR2"});

    for(auto k : v){
        OptionChain.add_row({
            toString(k.bsr1), toString(k.oi1), toString(k.volume1), toString(k.change1),
            toString(k.ltp1), to_string(k.strike), toString(k.ltp2),
            toString(k.change2), toString(k.volume2), toString(k.oi2), toString(k.bsr2)
        });   
    }

    OptionChain.format().corner("+")
    .border_top("-").border_bottom("-")
    .border_left("|").border_right("|");
    
    int OCsize = OptionChain.size();
    for(size_t i = 2; i < OCsize - 1; i++)
        OptionChain[i].format()
            .hide_border_top()
            .hide_border_bottom();

    OptionChain[OCsize - 1].format()
        .hide_border_top()
        .border_bottom("-");
    
    OptionChain[0].format()
        .font_style({FontStyle::bold});

    for(size_t i = 0; i < OCsize; i++){
        OptionChain[i].format()
        .font_align(FontAlign::center);
    }

    cout << OptionChain << endl;
}

void TablePlotter::displayTable(OptionChainCreater& f) {
    vector<OptionChainFormat> v = f.m_OC;
    prepareTable(v);
}

void TablePlotter::displayTable(vector<OptionChainFormat>& v) {
    prepareTable(v);
}
