#include<vector>
#include<map>
#include<string>

using namespace std;

class Record{
	public:
		double time;
		double level1;
		double level2;
		double load;
		
		Record();
		Record(double _time, double l1, double l2);
		Record(double _time, double l1, double l2, double _load);
		
};

class Load{
	public:
		double time;
		double util;
		double level;
	
        Load();
		Load(double _time, double _util, double _level);
		
};

class FileProfile{
	public:
		map<string, int> accu;
		vector<Load> loads;
		vector<Record> records;
		int count;
		
		FileProfile();
		//FileProfile(map<string, int> accu, list<Record> records, int c);
};
