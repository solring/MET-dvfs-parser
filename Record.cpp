#include "Record.h"

using namespace std;

Record::Record(){
    time = 0.0;
    level1 = 0;
    level2 = 0;
    load = -1;
}

Record::Record(double _time, double l1, double l2){
    time = _time;
    level1 = l1;
    level2 = l2;
    load = -1;
}

Record::Record(double _time, double l1, double l2, double _load){
    time = _time;
    level1 = l1;
    level2 = l2;
    load = _load;
}

Load::Load(){
    time = 0.0;
    util = -1;
    level = 0.0;
}

Load::Load(double _time, double _util, double _level){
    time = _time;
    util = _util;
    level = _level;
}

FileProfile::FileProfile(){
    count = 0;
}
