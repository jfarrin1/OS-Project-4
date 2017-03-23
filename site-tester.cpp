#include "curl.h"
#include "parseData.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <string.h>
#include <queue>
#include <cmath>
#include <pthread.h>
#include <errno.h>
#include <new>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
using namespace std;

int PERIOD_FETCH = 180;
int NUM_FETCH = 1;
int NUM_PARSE = 1;
string SEARCH_FILE = "Search.txt";
string SITE_FILE = "Sites.txt";
pthread_mutex_t fMutex, pMutex;
pthread_cond_t pCond, fCond;
jmp_buf env;

struct siteData{
	string site;
	string data;

};

typedef struct __fetch_args {
	queue<string> fetchQ;
	queue<siteData> parseQ;
} fetch_args;

void * threadFetch(void*);
void timer_reset(int);

int main(int argc, char* argv[]){
	signal(SIGALRM, timer_reset);
	fetch_args ourData;


	string line;
	string delim = "=";
	if (argc <2){
		cout<<"Please give a config file"<<endl;
		return 0;
	}
	fstream cFile(argv[1]);
	while(cFile>>line){
		size_t pos=0;
		string split;
		while((pos = line.find(delim)) != std::string::npos){
			split = line.substr(0,pos);
			line.erase(0,pos+delim.length());
		}
		string val = line;
		if(split.compare("PERIOD_FETCH")==0){
			PERIOD_FETCH = stoi(val);
		} else if(split.compare("NUM_FETCH")==0){
			NUM_FETCH = stoi(val);
		} else if(split.compare("NUM_PARSE")==0){
			NUM_PARSE = stoi(val);
		} else if(split.compare("SEARCH_FILE")==0){
			SEARCH_FILE = val;
		} else if(split.compare("SITE_FILE")==0){
			SITE_FILE = val;
		} else {
			cout<<"Error: Unkown parameter"<<endl;
		}
	}
	//Get Search strings=======================
	fstream searchFile(SEARCH_FILE);
	vector<string> searchStrings;
	while(getline(searchFile, line)){
		searchStrings.push_back(line);
	}
	int val = setjmp(env);
	alarm(PERIOD_FETCH);
	//get site html content
	fstream siteFile(SITE_FILE);
	pthread_t * pArray = new pthread_t[NUM_FETCH];
	while(siteFile>>line){
		ourData.fetchQ.push(line);
		pthread_cond_signal(&fCond);
	}
		//start threading
		cout<<"have data"<<endl;
		for(int i=0; i<NUM_FETCH; i++){
			cout<<"creating"<<endl;
			pthread_create(&pArray[i], NULL, threadFetch, (void*) &ourData);
		}
		for(int i=0; i<NUM_FETCH; i++){
			cout<<"joining"<<endl;
			int rc = pthread_join(pArray[i], NULL);
			if(rc!=0){
				cout<<"it done fucked up"<<endl;
			}
		}
	while(!ourData.parseQ.empty()){
		 parseData(searchStrings, ourData.parseQ.front().data,"output.txt",ourData.parseQ.front().site);
		 ourData.parseQ.pop();
	}
	delete [] pArray;
	return 0;
}

void timer_reset(int s){
	longjmp(env, 0);
}


void *threadFetch(void *fData) {
	fetch_args * pData = (fetch_args *) fData;
//	while(1) { // or while(gKeepRunning)
		// lock Mutex
		pthread_mutex_lock(&fMutex);
		while (pData->fetchQ.empty()) { 	// bars other threads from using
			pthread_cond_wait(&fCond, &fMutex);
		}
		// pop the first item from queue
		string current_URL = pData->fetchQ.front();
		pData->fetchQ.pop();
		// unlock fqueue mutex
		pthread_mutex_unlock(&fMutex);

		// CURL

		string siteHTML = mainCurl(current_URL);
		string site = current_URL;
		siteData temp {site, siteHTML};
		// lock parse queue
		pthread_mutex_lock(&pMutex);

		// put data/work item in parse queue
		pData->parseQ.push(temp);

		// signal or broadcast (bcast preferred way)
		pthread_cond_broadcast(&pCond);
		// unlock parse queue
		pthread_mutex_unlock(&pMutex);
//	}
	return NULL;
}
