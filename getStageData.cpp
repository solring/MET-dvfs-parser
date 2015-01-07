
using namespace std;
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<map>

#include<dirent.h>
#include<string.h>

#define STR_BUFFER_SIZE 128
#define MAX_CORE_NUM 8


typedef struct section{
	double start;
	double end;
	double duration;
	double total_load;
	double total_mem;
	double avg_load[MAX_CORE_NUM];
	double avg_freq[MAX_CORE_NUM];
	double avg_mem;
}Section;

void outputResult(map<string, vector<Section> > flist, int core_num){
    fstream fdout;
    char buf[STR_BUFFER_SIZE];
    fdout.open("Summary.csv", ios::out);

    for(map<string, vector<Section> >::iterator it = flist.begin(); it!=flist.end(); it++){
        string key = string(it->first);
        vector<Section> secs = it->second;
		
		//header
		fdout << key << endl;
		fdout << "start,end,duration,avg_mem"; 
		for(int j=0; j<core_num; j++) fdout << ",avg_freq" << j << ",avg_load" << j;
		fdout << endl;
		
		//for every section
		for(int i = 0; i < secs.size(); i++){
			
			sprintf(buf, "%lf,%lf,%lf,%lf", secs[i].start, secs[i].end, secs[i].duration, secs[i].avg_mem);
			fdout << buf;
			
			for(int j=0; j<core_num; j++) {
				sprintf(buf, ",%lf,%lf", secs[i].avg_freq[j], secs[i].avg_load[j]);
				fdout << buf;
			}
			fdout << endl;
		
		}
		fdout << endl;
    }
    fdout.close();
}

vector<Section> getSections(string &filename){
	fstream fd;
    char buf[STR_BUFFER_SIZE];
    double timestamp;
    double last = -1;
	vector<Section> secs;
	
	cout << "getSections: Parsing file " << filename << endl;
    fd.open(filename.c_str(), ios::in);
	
	while(fd.getline(buf, sizeof(buf), '\n')){
		if(sscanf(buf, "%lf", &timestamp) < 1){
			cout << "ERROR: getSections read line failed." << endl;
			continue;
		}
		if(last == -1){ // the first record
			last = timestamp;
			continue;
		}
		
		Section s = {
			.start = last,
			.end = timestamp,
			.duration = timestamp - last,
			.total_load = 0,
			.total_mem = 0,
			.avg_load = {},
			.avg_freq = {},
			.avg_mem = 0
		};
		
		last = timestamp;
		secs.push_back(s);		
	}
	return secs;
}

/********* Get section files *********/
int getfiles(string dir, vector<string> &files){
    DIR *dp;
    struct dirent *dirp;
    
    if((dp = opendir( dir.c_str() )) == NULL){
        cout << "  ERROR: getfiles: dir " << dir << "open fail (" << errno << endl;
        return errno;
    }

    while((dirp = readdir(dp)) != NULL){
        //join the dir and file name

		if(strstr(dirp->d_name, ".sec") == NULL) continue;
		
		string s = string(dir);
        if(s.find_last_of('\\') != s.size()) 
            s.append("\\");

        s.append(dirp->d_name);
        files.push_back(s);
		cout << "target file: " << s << endl;
    }
    closedir(dp);
    return 0;
}

void parseMemLoads(string memfile, vector<Section> &sections, int sample_rate){
	fstream fdmem;
	char buf[STR_BUFFER_SIZE];
	double timestamp, load;
    double mem_accu;
	double period = (double)1/sample_rate;
	int stage = 0;
	
	//open file
	fdmem.open(memfile.c_str(), ios::in);
	
	//parse mem file
	cout << "parsing memory load file " << memfile << endl;
	mem_accu = 0;
	stage = 0;
	while(fdmem.getline(buf, sizeof(buf), '\n')){
		if(stage > sections.size()) break;
		
		if(sscanf(buf, "%lf,%lf", &timestamp, &load) < 1){
            cout << "getCpuMemLoads: read timestamp & load fail." << endl;
            continue; 
        }
		
		if(timestamp > sections[stage].end){
			double diff = sections[stage].duration;
			if(diff < period) diff = period;
			sections[stage].total_mem = mem_accu;
			sections[stage].avg_mem = mem_accu / (diff * sample_rate);
			mem_accu = load;
			stage += 1;
		}else{
			mem_accu += load;
		}
	}

}

void parseCpuLoads(string dvfsfile, string loadfile, vector<Section> &sections, int sample_rate, int core){
	
	fstream fddvfs, fdload;
	char buf[STR_BUFFER_SIZE];
	double timestamp, load, last_freq = -1, last_swt = -1;
    double dvfs_accu, load_accu;
	double period = (double)1/sample_rate;
	int stage = 0;
	int start = 1;
	
	fddvfs.open(dvfsfile.c_str(), ios::in);
	fdload.open(loadfile.c_str(), ios::in);
	
	//parse load file and calculate the average load
	cout << "parsing CPU load file " << loadfile << endl;
	load_accu = 0;
	stage = 0;
	while(fdload.getline(buf, sizeof(buf), '\n')){
		if(stage > sections.size()) break;
		
		if(sscanf(buf, "%lf,%lf", &timestamp, &load) < 1){
            cout << "parseCpuLoads: read timestamp & load fail." << endl;
            continue; 
        }
		
		//fix start time of the first stage if it exceeds the range of log
		if(start){
			start = 0;
			if(timestamp > sections[stage].start){
				sections[stage].start = timestamp;
				sections[stage].duration = sections[stage].end - timestamp;
			}
		}
		
		if(timestamp > sections[stage].end){
			double diff = sections[stage].duration;
			if(diff < period) diff = period;
			sections[stage].total_load = load_accu;
			sections[stage].avg_load[core] = load_accu / (diff * sample_rate);
			load_accu = load;
			stage += 1;
		}else{
			load_accu += load;
		}
	}
	//fix end time of the last stage if it exceeds the range of los
	if(stage < sections.size() && timestamp < sections[stage].end) {
		sections[stage].end = timestamp;
		sections[stage].duration = timestamp - sections[stage].start;
	}

	//******************************************************************
	//parse dvfs file and calculate average freq. of the stage
	cout << "parsing CPU freq file " << dvfsfile << endl;
	dvfs_accu = 0;
	stage = 0;
	while(fddvfs.getline(buf, sizeof(buf), '\n')){
		if(stage >= sections.size()) break;
		
		if(sscanf(buf, "%lf,%lf", &timestamp, &load) < 1){
            cout << "parseCpuLoads: read timestamp & load fail." << endl;
            continue; 
        }
		
		//first record, no freq. switch
		if(last_freq == -1){
			last_freq = load;
			last_swt = sections[stage].start; // assume the freq. before first record is the same
			continue;
		}
		
		// to next stage, and calculate the average freq. first
		while(timestamp > sections[stage].end){ 
			double diff = (sections[stage].start > last_swt) ? sections[stage].duration : sections[stage].end - last_swt;
			//if(diff < period) diff = period;
			dvfs_accu += last_freq * diff;
			
			sections[stage].avg_freq[core] = dvfs_accu / sections[stage].duration;
			dvfs_accu = 0;
			stage += 1;
		}
		
		//if freq. switch happens
		if(stage < sections.size() && load != last_freq){ 
			double diff = sections[stage].start > last_swt ? timestamp - sections[stage].start : timestamp - last_swt;
			dvfs_accu += last_freq * diff;
			
			last_freq = load;
			last_swt = timestamp;
		}
		
	}
	while(stage < sections.size()){ //the last switch is earlier than the end time of the final stage
		double diff = sections[stage].start > last_swt ? sections[stage].duration : sections[stage].end - last_swt;
		dvfs_accu += last_freq * diff;
		
		sections[stage].avg_freq[core] = dvfs_accu / sections[stage].duration;
		dvfs_accu = 0;
		stage += 1;
	}

}

int main(int argc, char **argv){
    
    int sample_rate = 50;
	int core_num = 2;
	char buf[50];
    //map<string ,FileProfile> fps;
	map<string, vector<Section> > fps;

    if(argc < 3){
        cout << "usage: getSectionData [input directory] [sample rate] [number of core]" << endl;
        return 0;
    }
    
    sscanf(argv[2], "%d", &sample_rate);
	sscanf(argv[3], "%d", &core_num);
    string targetdir = string(argv[1]);
    printf("sample rate: %d\n", sample_rate);
    cout << "target directory: " << targetdir << endl;
	
	//get section files
	vector<string> files;
    if(getfiles(targetdir, files)) {
		cout << "ERROR: get section files failed." << endl;
	}
	printf("size of filelist: %d\n", files.size());
   
    //for every section file, parse the switches and store to the vector.
    for(int i=0; i<files.size(); i++){
		
		int pos = files[i].find(".sec");
		string root = files[i].substr(0, pos);
		fps[root] = getSections(files[i]);
		
		// for every core
		for(int j=0; j<core_num; j++){
			//get the DVFS and CPU load file names
			sprintf(buf, ".xlsx_DVFS.CPU-%d (kHz).csv", j);
			string dvfsfile = string(root);
			dvfsfile.append(buf);
			sprintf(buf, ".xlsx_LOAD.CPU-%d.csv", j);
			string loadfile = string(root);
			loadfile.append(buf);

			parseCpuLoads(dvfsfile, loadfile, fps[root], sample_rate, j);
		}
		string memfile = string(root);
		memfile.append(".xlsx_EMI_MM (MB s).csv");
		parseMemLoads(memfile, fps[root], sample_rate);
    }
    outputResult(fps, core_num);
    return 0;
}
