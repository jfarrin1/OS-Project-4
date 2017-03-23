#include <iostream>
#include <string>
#include <cmath>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
//#include <pthread.h>
#include <curl/curl.h>
#include <errno.h>
//#include <signal.h>

using namespace std;

//mutex_t fMutex;
//condition_variable cv;
//queue <string> fetch_queue;

mutex_t fMutex, pMutex;
cond_t pCond, fCond;


void *threadFetch(void *pData) {
	queue<string> fetchQ = (queue<string>) pData;
	while(1) { // or while(gKeepRunning)
		// lock Mutex
		pthread_mutex_lock(&fMutex);
		while (fetchQ.empty()) { 	// bars other threads from using
			pthread_cond_wait(&fCond, &fMutex);
		}
		// pop the first item from queue
		string current_URL = fetchQ.front()
		fetchQ.pop();
		// unlock fqueue mutex
		pthread_mutex_unlock(&fMutex);

		// CURL

		string siteHTML = mainCurl(pData.site);
		string site = pData.site;
		siteData temp{site, siteHTML);
		// lock parse queue
		pthread_mutex_lock(&pMutex);

		// put data/work item in parse queue
		pData.parseQ.push(temp);

		// signal or broadcast (bcast preferred way)
		pthread_cond_broadcast(&pCond);
		// unlock parse queue
		pthread_mutex_unlock(&pMutex);
	}

}
