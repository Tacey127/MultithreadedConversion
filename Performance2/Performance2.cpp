// Performance2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Performance2.h"

#include <iostream>

//ADDED STUFF
#include <stdio.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <thread>
#include <ppl.h>
#include <concurrent_vector.h>
#include <ctime>

using namespace std;
using namespace concurrency;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Timer - used to established precise timings for code.
class TIMER
{
	LARGE_INTEGER t_;

	__int64 current_time_;

	public:
		TIMER()	// Default constructor. Initialises this timer with the current value of the hi-res CPU timer.
		{
			QueryPerformanceCounter(&t_);
			current_time_ = t_.QuadPart;
		}

		TIMER(const TIMER &ct)	// Copy constructor.
		{
			current_time_ = ct.current_time_;
		}

		TIMER& operator=(const TIMER &ct)	// Copy assignment.
		{
			current_time_ = ct.current_time_;
			return *this;
		}

		TIMER& operator=(const __int64 &n)	// Overloaded copy assignment.
		{
			current_time_ = n;
			return *this;
		}

		~TIMER() {}		// Destructor.

		static __int64 get_frequency()
		{
			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency); 
			return frequency.QuadPart;
		}

		__int64 get_time() const
		{
			return current_time_;
		}

		void get_current_time()
		{
			QueryPerformanceCounter(&t_);
			current_time_ = t_.QuadPart;
		}

		inline bool operator==(const TIMER &ct) const
		{
			return current_time_ == ct.current_time_;
		}

		inline bool operator!=(const TIMER &ct) const
		{
			return current_time_ != ct.current_time_;
		}

		__int64 operator-(const TIMER &ct) const		// Subtract a TIMER from this one - return the result.
		{
			return current_time_ - ct.current_time_;
		}

		inline bool operator>(const TIMER &ct) const
		{
			return current_time_ > ct.current_time_;
		}

		inline bool operator<(const TIMER &ct) const
		{
			return current_time_ < ct.current_time_;
		}

		inline bool operator<=(const TIMER &ct) const
		{
			return current_time_ <= ct.current_time_;
		}

		inline bool operator>=(const TIMER &ct) const
		{
			return current_time_ >= ct.current_time_;
		}
};

CWinApp theApp;  // The one and only application object

using namespace std;

struct entry {
	string sessionID;
	string ipAdress;
	string browser;
	vector<string> path;
	vector<string> time;
	float timeDuration;
};


vector<entry*> entries;
mutex mtx;
void StoreEntry(entry* newEntry) 
{
	mtx.lock();
	entries.push_back(newEntry);
	mtx.unlock();
}

void ChunkData(pair<size_t, size_t> &chunk, char* buffer)
{
	//ChunkData(chunk, buffer); 
	entry* newEntry = new entry;
	for (size_t i = chunk.first; i < chunk.second; ++i)
	{

		if (buffer[i] == '<')
		{
			++i;
			switch (buffer[i])
			{
				//case 'e':
				//	entries.push_back(new entry);
				//	break;
			case 's':
			{

				i += 10;

				string seshID;
				while (buffer[i] != '<')
				{
					seshID += buffer[i];
					++i;

				}
				newEntry->sessionID = seshID;

			}
			break;
			case 'i'://IP adress
			{
				i += 10;
				string ipAd;
				while (buffer[i] != '<')
				{
					ipAd += buffer[i];
					++i;
				}

				newEntry->ipAdress = ipAd;

			}
			break;
			case 'b'://browser
			{
				i += 8;
				string br;
				while (buffer[i] != '<')
				{
					br += buffer[i];
					++i;
				}

				newEntry->browser = br;

			}
			break;
			case 'p':
			{
				i += 5;
				string bu;
				while (buffer[i] != '<')
				{
					bu += buffer[i];
					++i;
				}
				newEntry->path.push_back(bu);
			}
			break;
			case 't':
			{
				i += 5;
				string t;
				while (buffer[i] != '<')
				{
					t += buffer[i];
					++i;
				}
				newEntry->time.push_back(t);
			}
			break;
			default:
				break;
			}
		}
	}

	//
	int mo, d, y, h, mi, s;
	int mo1, d1, y1, h1, mi1, s1;
	char sa;
	sscanf(newEntry->time.at(0).c_str(), "%d %c %d %c %d %d %c %d %c %d", &d, &sa, &mo, &sa, &y, &h, &sa, &mi, &sa, &s);
	sscanf(newEntry->time.back().c_str(), "%d %c %d %c %d %d %c %d %c %d", &d1, &sa, &mo1, &sa, &y1, &h1, &sa, &mi1, &sa, &s1);
	s1 -= s;
	s1 += (mi1 - mi) * 60;
	s1 += (h1 - h) * 3600;

	//https://www.daniweb.com/programming/software-development/threads/7813/date-difference-in-c
	struct std::tm date1 = { 0,0,0,d,mo,y - 1900 };
	struct std::tm date2 = { 0,0,0,d1,mo1,y1 - 1900 };
	std::time_t xt = std::mktime(&date1);
	std::time_t yt = std::mktime(&date2);
	if (xt != (std::time_t)(-1) && yt != (std::time_t)(-1))
	{
		double difference = std::difftime(yt, xt) / (60 * 60 * 24);

		s1 += difference * 86400;
	}
	newEntry->timeDuration = s1;
	StoreEntry(newEntry);
}

bool CompareIPAdress(entry* entryOne, entry* entryTwo) 
{
	return entryOne->ipAdress < entryTwo->ipAdress;
}

void SaveBasicEntries(vector<entry*> entries)
{
	fstream file;
	file = fstream("log.json", ios::out);
	int i = 0;
	string e = "{\n \"entry\" : [\n";
	for each (entry* en in entries)
	{

		//e += "\"entry\" : {\n";

		e += "{\n\"sesstionid\": \"" + en->sessionID + "\",\n";
		e += "\"ipadress\": \"" + en->ipAdress + "\",\n";
		e += "\"browser\": \"" + en->browser + "\",\n";
		//path
		e += "\"path\": [";
		e += "\"" + en->path.at(0) + "\"";
		for (size_t x = 1; x < en->time.size(); ++x)
		{
			e += ", \"" + en->path.at(x) + "\"";
		}
		e += "]\n";

		//time
		e += "\"time\": [";
		e += "\"" + en->time.at(0) + "\"";
		for (size_t y = 1; y < en->time.size(); ++y)
		{
			e += ", \"" + en->time.at(y) + "\"";
		}
		
		e += "]\n},\n";
		++i;
		if (i > 50000)
		{
			file << e;
			i = 0;
			e = "";
		}
	}
	e += "\n]\n}";
	file << e;
	file.close();
}

void SaveStatistics(vector<entry*> entries)
{
	fstream sfile;
	sfile = fstream("stats.json", ios::out);


	vector<int> times;

	string comp = entries[0]->ipAdress;


	//average time users spend on the site
	int number = 0;
	int avg = 0;
	//the number of times users with the same ip adress visit the site
	string e = "{\n \"entry\" : [\n";
	for each (entry* en in entries)
	{
		if (comp == en->ipAdress)
		{
			number++;
			times.push_back(en->timeDuration);
			avg += en->timeDuration;
		}
		else
		{

			e += "{\"ipadress\": \"" + comp + "\",\n";
			e += "\"timesvisited\": \"" + to_string(number) + "\",\n";
			//time duration for each session
			e += "\"sessionduration\": [";

			e += "\"" + to_string(times.at(0)) + "\"";
			for (size_t y = 1; y < en->time.size(); ++y)
			{
				e += ", \"" + to_string(times.at(y)) + "\"";
			}

			e += "]\n";
					   
			e += "\"averagetimespent\": \"" + to_string(avg / number) + "\"\n";
			
			e += "},\n";
			sfile << e;
			e = "";

			number = 1;
			times.clear();
			times.push_back(en->timeDuration);
			comp = en->ipAdress;
			avg = en->timeDuration;
		}

	}
	e += "\n]\n}";
	sfile << e;
	sfile.close();
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize Microsoft Foundation Classes, and print an error if failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// Application starts here...

		// Time the application's execution time.
		TIMER start;	// DO NOT CHANGE THIS LINE. Timing will start here.

		//--------------------------------------------------------------------------------------
		// Insert your code from here...
		
		struct stat statbuf;
		stat("log.xml", &statbuf);
		//statbuf.st_size;//THE MEMORY BUFFER
		FILE * pFile = fopen("log.xml", "rb");
		char * buffer = (char*) malloc(statbuf.st_size);

		fread(buffer, 1, statbuf.st_size, pFile);
		fclose(pFile);
		
		vector<pair<size_t, size_t>> chunk;
		//read data into chunks
		size_t j = 0;
		for (size_t i = 0; i < statbuf.st_size; ++i) {
			//find the \n
			//send the line to be processed via multithreading
			if (buffer[i] == '\n') 
			{
				chunk.push_back(pair<size_t, size_t>(j, i));
				j = i;
			}
		}

		parallel_for_each(chunk.begin(), chunk.end(), [&](pair<size_t, size_t> chunk) {
			ChunkData(chunk, buffer);
		});
		
		//
		sort(entries.begin(), entries.end(), CompareIPAdress);
		//save basic file
		thread SB (SaveBasicEntries, entries);
		//save statistics file
		thread SST(SaveStatistics, entries);
		
		//the time duration for each session
		//the average time users spend on the site
		//the number of times users with the same ip adress visit the site
		free(buffer);
		SB.join();
		SST.join();
		for (entry* entyptr : entries) 
		{
			delete entyptr;
		}
		entries.clear();
			//-------------------------------------------------------------------------------------------------------
			// How long did it take?...   DO NOT CHANGE FROM HERE...

		TIMER end;

		TIMER elapsed;

		elapsed = end - start;

		__int64 ticks_per_second = start.get_frequency();

		// Display the resulting time...

		double elapsed_seconds = (double)elapsed.get_time() / (double)ticks_per_second;

		cout << "Elapsed time (seconds): " << elapsed_seconds;
		cout << endl;
		cout << "Press a key to continue" << endl;

		char c;
		cin >> c;
	}

	return nRetCode;
}
