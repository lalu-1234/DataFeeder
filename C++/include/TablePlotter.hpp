#pragma once

#include "OptionChainCreater.hpp"
#include <tabulate/table.hpp>

using namespace tabulate;

class TablePlotter{
public:
    void prepareTable(vector<OptionChainFormat>& v);
    
    string toString(double x);

    void displayTable(OptionChainCreater& f);
    void displayTable(vector<OptionChainFormat>& v);
};
