#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
using namespace std;

int parseData(vector<string> words,string data, string oFile, string site){
	ofstream output;
	output.open(oFile, std::ios_base::app);
	for(size_t i=0; i < words.size(); i++){
		int count=0;
		size_t nPos = data.find(words[i], 0);
		while(nPos != string::npos){
			count++;
			nPos = data.find(words[i], nPos+1);
		}
		chrono::time_point<std::chrono::system_clock> now;
		now = chrono::system_clock::now();
		time_t now_time = chrono::system_clock::to_time_t(now);
		string newTime = ctime(&now_time);
		newTime.erase(newTime.end()-1,newTime.end());
		output<<newTime<<","<<words[i]<<","<<site<<","<<count<<","<<endl;
	}
	output.close();
	return 0;
}
