#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<map>
#include<list>

#include<dirent.h>
#include<string.h>
#include"Record.h"


#define STR_BUFFER_SIZE 128

using namespace std;

int getfiles(string dir, vector<string> &files){
    DIR *dp;
    struct dirent *dirp;
    
    if((dp = opendir( dir.c_str() )) == NULL){
        cout << "ERROR: dir " << dir << "open fail (" << errno << endl;
        return errno;
    }

    while((dirp = readdir(dp)) != NULL){
        //join the dir and file name
		
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

void outputResult(map<string, FileProfile> flist){
    fstream fdout;
    char buf[STR_BUFFER_SIZE];
    fdout.open("Summary.csv", ios::out);

    for(map<string, FileProfile>::iterator it = flist.begin(); it!=flist.end(); it++){
       
        string key = string(it->first);
        FileProfile fp = it->second;
        fdout << key << endl;
        
        //Switch Summary
        fdout << "Switch,Count" << endl;
#if __GNUC__ > 4
        for(pair<string, int> it : fp.accu){
            sprintf(buf, "%s,%d", it.first.c_str(), it.second);
            fdout << buf << endl;
		}
#else
        for(map<string, int>::iterator it = fp.accu.begin(); it!=fp.accu.end(); it++){
            sprintf(buf, "%s,%d", it->first.c_str(), it->second);
            fdout << buf << endl;
        }
#endif
        fdout << endl;
    
        //Detail
        fdout << "Time,Freq1,Freq2,Load" << endl;
#if __GNUC__ > 4
		for(Record it : fp.records){
			sprintf(buf, "%lf,%lf,%lf,%lf", it.time, it.level1, it.level2, it.load);
			fdout << buf << endl;
		}
#else
        for(vector<Record>::iterator it = fp.records.begin(); it!=fp.records.end(); it++){
            sprintf(buf, "%lf,%lf,%lf,%lf", it->time, it->level1, it->level2, it->load);
            fdout << buf << endl;
        }
#endif
        fdout << endl;

        //Detail load
        fdout << "Time,Freq,Load" << endl;
#if __GNUC__ > 4
		for(Load it : fp.loads){
			sprintf(buf, "%lf,%lf,%lf", it.time, it.level, it.util);
            fdout << buf << endl;
		}
#else
        for(vector<Load>::iterator it = fp.loads.begin(); it!=fp.loads.end(); it++){
            sprintf(buf, "%lf,%lf,%lf", it->time, it->level, it->util);
            fdout << buf << endl;
        }
#endif
        fdout << endl;
    }
    fdout.close();
}

void parseUtil(FileProfile &fp, string &loadfile, int sample_rate){
    cout << "parseUtil: parsing file " << loadfile << endl;
    
    fstream fd;
    char buf[STR_BUFFER_SIZE];
    int ptr = 1; //skip the first of record which does not indicate a switch of freq.
    double timestamp, utili = 0;
    double swt_t, last_swt_t = 0;
    double load, accu;
    int count = 0;
    
    double period = 1.0/(double)sample_rate;
    int maxlen = fp.records.size();


    if(maxlen < 2){
        swt_t = -1;
    }else{
        swt_t = fp.records[1].time; //skip the first of record which does not indicate a switch of freq.
        
    }
   
    // open the load trace
    fd.open(loadfile.c_str(), ios::in);

    while(fd.getline(buf, sizeof(buf), '\n')){
        if(strstr(buf, "timestamp") != NULL) continue; //header
      
        // get timestamp, load
        if(sscanf(buf, "%lf,%lf", &timestamp, &load) < 1){
            cout << "parseUtil: read timestamp & load fail." << endl;
            continue; 
        }

        if(last_swt_t == 0) //begin of trace
            last_swt_t = timestamp;


        if(timestamp > swt_t && ptr < maxlen){ //switch!
            cout << "switch!! " << endl;
        printf("timestamp: %.2lf, last_swt_t: %.2lf, load: %.2lf\n", timestamp, last_swt_t, load);
        printf("ptr = %d, maxlen = %d\n", ptr, maxlen);

            double diff = swt_t - last_swt_t;
            
            if(diff < period){
                fp.records[ptr].load = accu;
            }else{
                fp.records[ptr].load = (accu / (diff * sample_rate));
            }
            
            //fp.records[ptr].load = accu / count;
            printf("diff: %lf, period: %lf, util: %.2lf\n", diff, period, fp.records[ptr].load);
            
            accu = load;
            count = 1;
            ptr += 1;

            //check if it's finished
            if(ptr < maxlen){
                last_swt_t = swt_t;
                swt_t = fp.records[ptr].time;
            }

        }else{ // not switch
            accu += load;
            count += 1;
        }

        //after finising reading load file 
        //append the load and freq to fp.loads
        fp.loads.push_back( Load(timestamp, load, fp.records[ptr-1].level2) );
    }//end read load trace

    //append the average util after the last switch
    double last_level = fp.records[maxlen-1].level2;
    double diff = timestamp - swt_t;
    if(diff < period){
        fp.records.push_back( Record(-1.0, last_level, -1.0, accu) );
    }else{
        fp.records.push_back( Record(-1.0, last_level, -1.0, accu / (diff * sample_rate )) );
    }
   
    fd.close();
}

FileProfile parseDVFSSwitch(string &targetf){
    fstream fd;
    char buf[STR_BUFFER_SIZE];
    char keybuf[STR_BUFFER_SIZE];
    FileProfile fprofile;
    double timestamp, start;
    double freq, last = -1;
    int res;

    cout << "parseDVFSSwitch: Parsing file " << targetf << endl;
    fd.open(targetf.c_str(), ios::in);

    while(fd.getline(buf, sizeof(buf), '\n')){
        
        if(strstr(buf, "timestamp") != NULL) continue; //header

        if(sscanf(buf, "%lf,%lf", &timestamp, &freq) < 1){
            cout << "parseDVFSSwitch: read timestamp & freq fail." << endl;
            continue;
        }
        //printf("timestamp: %lf, freq: %lf\n", timestamp, freq);

        if(last == -1){ //begin of the trace
            start = timestamp;
            last = freq;
            fprofile.records.push_back(Record(timestamp, -1, freq));
            continue;
        }

        if(last!=freq){
            
            sprintf(keybuf, "%lf-%lf", last, freq);
            string key = string(keybuf);

            //accumulate the # of switch 
            if(fprofile.accu.find(key) == fprofile.accu.end()){
                fprofile.accu[key] = 1;
            }else{
                fprofile.accu[key] += 1;
            }
            //push new switch record
            fprofile.records.push_back(Record(timestamp, last, freq));

            last = freq;
            fprofile.count += 1;
        } 
    }

    fd.close();
    return fprofile;
}

int main(int argc, char **argv){
    
    int sample_rate = 50;
    map<string ,FileProfile> fps;

    if(argc < 3){
        cout << "usage: parseDFSswt [input directory] [sample rate]" << endl;
        return 0;
    }
    
    sscanf(argv[2], "%d", &sample_rate);
    string targetdir = string(argv[1]);
    printf("sample rate: %d\n", sample_rate);
    cout << "target directory: " << targetdir << endl;

    vector<string> files;
    getfiles(targetdir, files);
	printf("size of filelist: %d\n", files.size());
   
    //for every DFVS file, parse the switches and store to the vector.
    for(int i=0; i<files.size(); i++){

        string *targetf;
        targetf = &files[i];
        if(targetf->find("DVFS")==string::npos) continue;
		
		cout << "before call parseDVFS" << endl;
        FileProfile fprofile = parseDVFSSwitch(*targetf);
		cout << "after call parseDVFS" << endl;
        
        printf("Number of switch: %d\n", fprofile.count);
        fps[*targetf] = fprofile;

        //get the load file name from DVFS file
        int index = targetf->find("DVFS");
        int index2 = targetf->find(" (kHz)");
        string loadfilename = string(*targetf);
        loadfilename.replace(index, 4, "LOAD").replace(index2, 6, "");
       
        parseUtil(fprofile, loadfilename, sample_rate);
        
        //push the file profile to the filelist
        fps[*targetf] = fprofile;
    }

    outputResult(fps);
    return 0;
}
