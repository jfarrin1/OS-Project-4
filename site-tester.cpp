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
int fKeepRunning = 1;
vector<string> searchStrings;

struct siteData{
	string site;
	string data;

};

typedef struct __fetch_args {
	queue<string> fetchQ;
	queue<siteData> parseQ;
} fetch_args;

fetch_args ourData;
pthread_t * pArray = new pthread_t[NUM_PARSE];
pthread_t * fArray = new pthread_t[NUM_FETCH];
string line;

void * threadFetch(void*);
void * threadParse(void*);
void timer_reset(int);
void interrupt_handler(int);


int main(int argc, char* argv[]){
	signal(SIGALRM, timer_reset);
	signal(SIGINT, interrupt_handler);
	signal(SIGHUP, interrupt_handler);
	fetch_args ourData;

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
			if (stoi(val) <0){
				cout<<"error: PERIOD_FETCH must be >=0"<<endl;
				return 1;
			}
			PERIOD_FETCH = stoi(val);
		} else if(split.compare("NUM_FETCH")==0){
			if (stoi(val) >8 || stoi(val) <1){
				cout<<"error: invalid NUM_FETCH"<<endl;
				return 1;
			}
			NUM_FETCH = stoi(val);
		} else if(split.compare("NUM_PARSE")==0){
			if (stoi(val) >8 || stoi(val) <1){
				cout<<"error: invalid NUM_PARSE"<<endl;
				return 1;
			}
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
	if(!searchFile.is_open()){
		cout<<"error: could not open "<<SEARCH_FILE<<endl;
		return 1;
	}
	while(getline(searchFile, line)){
		searchStrings.push_back(line);
	}
	//get site html content
	fstream siteFile(SITE_FILE);
	if(!siteFile.is_open()){
		cout<<"error: could not open "<<SITE_FILE<<endl;
		return 1;
	}
	for(int i=0; i<NUM_FETCH; i++){
		cout<<"creating fetch thread"<<endl;
		pthread_create(&fArray[i], NULL, threadFetch, (void*) &ourData);
	}
	for(int i=0; i<NUM_FETCH; i++){
		cout<<"creating parse thread"<<endl;
		pthread_create(&pArray[i], NULL, threadParse, (void*) &ourData);
	}
	timer_reset(0);
	while(fKeepRunning);
	return 0;
}

void interrupt_handler(int s){
	cout<<"Ctrl C Received"<<endl;
	fKeepRunning = 0;
	pthread_cond_broadcast(&fCond);
	pthread_cond_broadcast(&pCond);
	for(int i=0; i < NUM_FETCH; i++){
		cout<<"joining fetch"<<endl;
		int rc = pthread_join(fArray[i], NULL);
		if (rc <0) cout<<"error joining"<<endl;
	}
	for(int i=0; i < NUM_PARSE; i++){
		int rc = pthread_join(pArray[i], NULL);
		if (rc<0) cout<<"error joining"<<endl;
	}
	delete [] fArray;
	delete [] pArray;
	exit(0);
}



void timer_reset(int s){
	alarm(PERIOD_FETCH);
	cout<<"timer triggered"<<endl;
	//get site html content
	fstream siteFile(SITE_FILE);
	while(siteFile>>line){
		ourData.fetchQ.push(line);
		cout<<"pushing to fetchQ"<<endl;
		pthread_cond_broadcast(&fCond);
	}
}

void *threadParse(void *fData) {
	fetch_args * pData = &ourData;
	while(fKeepRunning) { // or while(gKeepRunning)
		string data;
		string site;
		// lock Mutex
		pthread_mutex_lock(&pMutex);
		while (pData->parseQ.empty()&& fKeepRunning) { 	// bars other threads from using
			cout<<"waiting to parse"<<endl;
			pthread_cond_wait(&pCond, &pMutex);
		}
		// pop the first item from queue
		if(fKeepRunning){
			cout<<"parseing"<<endl;
			data = pData->parseQ.front().data;
			site = pData->parseQ.front().site;
			pData->parseQ.pop();
		}
		// unlock parse queue
		pthread_mutex_unlock(&pMutex);
		if (fKeepRunning){
			parseData(searchStrings, data,"output.txt",site);
		}
	}
	return NULL;
}



void *threadFetch(void *fData) {
	fetch_args * pData = &ourData;
	while(fKeepRunning) { // or while(gKeepRunning)
		string current_URL;
		string site;
		string siteHTML;
		siteData temp;
		// lock Mutex
		pthread_mutex_lock(&fMutex);
		while (pData->fetchQ.empty() && fKeepRunning) { 	// bars other threads from using
			cout<<"waiting to fetch"<<endl;
			pthread_cond_wait(&fCond, &fMutex);
		}
		// pop the first item from queue
		if (fKeepRunning){
			current_URL = pData->fetchQ.front();
			pData->fetchQ.pop();
		}
			// unlock fqueue mutex
		pthread_mutex_unlock(&fMutex);
		if (fKeepRunning){
			// CURL
			siteHTML = mainCurl(current_URL);
			site = current_URL;
			temp.site = site;
			temp.data = siteHTML;
		}
		// lock parse queue
		pthread_mutex_lock(&pMutex);
		if (fKeepRunning){
			// put data/work item in parse queue
			pData->parseQ.push(temp);
			cout<<"pushing to parseQ"<<endl;
		}
		// signal or broadcast (bcast preferred way)
		// unlock parse queue
		pthread_mutex_unlock(&pMutex);
		pthread_cond_broadcast(&pCond);
	}
	return NULL;
}
