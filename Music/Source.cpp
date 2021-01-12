#pragma comment(lib, "ffmpeg-cpp.lib")
#pragma comment(lib, "Winmm.lib")
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>
#include <mmsystem.h>
#include <TlHelp32.h>
#include "discord/discord.h"
#include "ffmpeg/ffmpegcpp.h"
#include "ui.h"

#define volume "50" // out of 100

struct DiscordState
{
	std::unique_ptr<discord::Core> core;
};

std::wstring name, nowplaying;
int next = -1, timesincestart;
bool paused = false, discordpaused, doneplaying, discordstarted = false, idle = false;
std::vector<std::string> supportedformats = { "mp3", "m4a", "wma", "flac" }; // must be lowercase
DiscordState state{};
discord::Core* core{};
discord::Activity activity{};
discord::ActivityTimestamps timestamp{};
std::wstring path;

int morerand(int n) // different pseudorandom number generator for shuffling
{
	long double cur = long double(rand() % 1000) / 1000;
	n = n % 10000;
	for (int i = 0; i < n; i++)
	{
		cur = abs(4.5864 * cur * (1 - cur));
	}
	return cur * 1000000;
}

void shuffle(std::vector<int>& curlist, int n) // shuffle songs
{
	int last, last2, last3, temp;
	std::set<int> used;
	if (curlist.size() > 3)
	{
		last = curlist[n - 1];
		last2 = curlist[n - 2];
		last3 = curlist[n - 3];
		used.insert(last);
		used.insert(last2);
		used.insert(last3);
	}
	std::random_device rd;
	std::mt19937 mt(rd());
	for (int i = 0; i < n; i++)
	{
		curlist[i] = -1;
	}
	srand(time(NULL) + clock());
	if (n >= 6)
	{
		curlist[abs(long long(mt() + morerand(rand()) + rd())) % (n - 3) + 3] = last;
		temp = abs(long long(mt() + rand() - rd() + morerand(mt()))) % (n - 3) + 3;
		while (curlist[temp] != -1)
		{
			temp = abs(long long(mt() + rand() + morerand(rd()))) % (n - 3) + 3;
		}
		curlist[temp] = last2;
		temp = abs(long long(rand() * mt() - rd() + morerand(time(NULL) + clock()))) % (n);
		while (curlist[temp] != -1)
		{
			srand(time(NULL));
			temp = abs(long long(mt() * morerand(rand() + rd()))) % (n);
		}
		curlist[temp] = last3;
	}
	for (int i = 0; i < n; i++)
	{
		if (used.find(i) != used.end())
		{
			continue;
		}
		temp = abs(long long(morerand(rand()) + mt() - rd())) % n;
		while (curlist[temp] != -1)
		{
			temp = abs(long long(morerand(mt() + rand()) + rd() + rd())) % n;
		}
		curlist[temp] = i;
		used.insert(i);
	}
}

void discordthing() // discord status
{
	std::wstring lastplaying;
	while (true)
	{
		std::string tempstr = "Playing ";
		for (int i = 0; i < nowplaying.length(); i++)
		{
			tempstr.push_back(nowplaying[i]);
		}
		lastplaying = nowplaying;
		activity.SetDetails(tempstr.c_str());
		timestamp.SetStart(time(NULL));
		activity.GetTimestamps() = timestamp;
		state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
		while (true)
		{
			// run callbacks
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			state.core->RunCallbacks();
			if (doneplaying)
			{
				activity.SetDetails("Idle");
				timestamp.SetStart(time(NULL));
				activity.GetTimestamps() = timestamp;
				state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
				idle = true;
			}
			if (idle && !doneplaying)
			{
				activity.SetDetails(tempstr.c_str());
				timestamp.SetStart(time(NULL));
				activity.GetTimestamps() = timestamp;
				state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
				idle = false;
			}
			if (nowplaying != lastplaying) // if song name is different, update status to new song
			{
				break;
			}
			if (paused && !discordpaused) // if paused, set status accordingly
			{
				activity.SetState("paused");
				timestamp.SetStart(time(NULL));
				activity.GetTimestamps() = timestamp;
				state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
				discordpaused = true;
			}
			if (discordpaused && !paused) // if unpaused, set status accordingly
			{
				activity.SetState("");
				timestamp.SetStart(time(NULL) - timesincestart / 1000); // go back to old timestamp (before paused)
				activity.GetTimestamps() = timestamp;
				state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
				discordpaused = false;
			}
		}
	}
}


void startdiscord() // start discord thing (duh)
{
	activity.SetType(discord::ActivityType::Playing);
	auto response = discord::Core::Create(777048406639116309, DiscordCreateFlags_Default, &core);
	state.core.reset(core);
	std::thread discth = std::thread(discordthing);
	discth.detach();
	discordstarted = true;
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	SetCurrentDirectoryW(path.c_str());
	mciSendString("close CURR_SND", NULL, 0, 0);
	system("del ^[ssmtemp^]*.wav");
	return FALSE;
}

bool getdiscord() // if Discord.exe is open
{
	DWORD pid = 0;
	PCSTR name = "Discord.exe";
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	if (Process32First(snapshot, &process))
	{
		do
		{
			if ((std::string)(process.szExeFile) == std::string(name))
			{
				pid = process.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
	if (pid != 0)
	{
		return true;
	}
	return false;
}



int main(int argc, char* argv[])
{
	SetConsoleTitle("Music");
	if (!SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		std::cout << "Ctrl handler failed." << std::endl;
		return -2;
	}
	ShowWindow(GetConsoleWindow(), SW_SHOW);
	int n = 0, ind = -1, thyme, lastbar, lastsec;
	const char* c;
	std::string temp, location, exepath;
	std::wstring str;
	std::vector<int> curlist, list;
	std::vector<std::wstring> v;
	std::wifstream fin;
	std::wofstream fout;
	exepath = argv[0];
	while (exepath[exepath.length() - 1] != '\\')
	{
		exepath.erase(exepath.end() - 1);
	}
	exepath.erase(exepath.end() - 1);
	if (argc > 1) // for "open with"
	{
		temp = argv[1];
		path = std::wstring(temp.begin(), temp.end());
		while (path[path.size() - 1] != '\\')
		{
			str.insert(str.begin(), path[path.size() - 1]);
			path.erase(path.end() - 1);
		}
		SetCurrentDirectoryW(path.c_str());
		curlist.push_back(0);
		v.push_back(str);
		n = 1;
		goto playing_start; // skip stuff bc we don't need it
	}
	fin.open("path");
	getline(fin, path); // get path name from file
	fin.close();
	if (path == L"") // if no path name in file, ask for path
	{
		std::cout << "What folder are your songs located in? [Or put the path in the \"path\" file in the sources dir]" << std::endl;
		std::getline(std::wcin, path);
		fout.open("path");
		fout << path << std::endl;
		fout.close();
		system("CLS");
	}
	SetCurrentDirectoryW(path.c_str()); // set directory to path
	for (const auto& entry : std::filesystem::directory_iterator(path)) // get files in path directory
	{
		str = entry.path().wstring();
		v.push_back(str);
		n++;
		curlist.push_back(n - 1);
	}
playing_start:
	thyme = clock(); // time when song starts
	if (getdiscord())
	{
		startdiscord();
	}
	while (true)
	{
		shuffle(curlist, n);
		next = -1;
		for (int i = 0; i < n; i++)
		{
			if (next == -1) // normal
			{
				if (i != (ind + 1) % n) // if out of sync (previous song), then get back in sync
				{
					next = list[ind];
					i--;
				}
				else
				{
					next = curlist[i];
					list.push_back(next);
				}
				ind++;
			}
			else if (next == -2)
			{
				if (ind > 0)
				{
					ind--;
					if (clock() - thyme >= 3000) // if more than 3 seconds since song start, go to start of song instead of prev song
					{
						ind++;
					}
					i--;
				}
				next = list[ind];
			}
			str = v[next];
			if (str.length() <= 4) // if filename is somehow shorter than 4 characters
			{
				continue;
			}
			name.clear();
			for (int j = str.length() - 1; j >= 0; j--) // set name to str without the path
			{
				if (str[j] == '\\')
				{
					break;
				}
				name.insert(name.begin(), str[j]);
			}
			system("CLS");
			name.erase(name.end() - 4, name.end()); // get rid of file extension
			std::string narrowstr, tempstr;
			tempstr.clear();
			narrowstr.clear();
			if (!(str[str.length() - 1] == 'v' && str[str.length() - 2] == 'a' && str[str.length() - 3] == 'w' && str[str.length() - 4] == '.')) // if str isn't a .wav file
			{
				int format = -1;
				for (int j = 0; j < supportedformats.size(); j++) // check if any of the supported formats matches
				{
					bool mismatch = false;;
					for (int k = 0; k < supportedformats[j].size(); k++)
					{
						if (str[str.length() - 1 - k] != supportedformats[j][supportedformats[j].size() - 1 - k])
						{
							if (str[str.length() - 1 - k] > 64 && str[str.length() - 1 - k] < 91)
							{
								if (str[str.length() - 1 - k] + 32 != supportedformats[j][supportedformats[j].size() - 1 - k])
								{
									mismatch = true;
									break;
								}
							}
							else
							{
								mismatch = true;
								break;
							}
						}
					}
					if (str[str.length() - 1 - supportedformats[j].size()] != '.')
					{
						mismatch = true;
					}
					if (mismatch) // if not current format
					{
						continue;
					}
					else // if current format
					{
						format = j;
						break;
					}
				}
				if (format == -1) // if format isn't supported, get new song
				{
					next = -1;
					continue;
				}
				/*for (int k = 0; k < name.length(); k++)
				{
					narrowstr.push_back(name[k]);
				}
				narrowstr += ".";
				narrowstr += supportedformats[format];
				tempstr = "[ssmtemp]";
				tempstr += narrowstr;
				tempstr.erase(tempstr.end() - 4, tempstr.end());
				tempstr += ".mp3";
				ffmpegcpp::Muxer* muxer = new ffmpegcpp::Muxer(tempstr.c_str());
				ffmpegcpp::AudioCodec* codec = new ffmpegcpp::AudioCodec(AV_CODEC_ID_MP3);
				ffmpegcpp::AudioEncoder* encoder = new ffmpegcpp::AudioEncoder(codec, muxer);
				ffmpegcpp::Filter* filter = new ffmpegcpp::Filter("volume=1", encoder);
				ffmpegcpp::Demuxer* audioFile = new ffmpegcpp::Demuxer(narrowstr.c_str());
				audioFile->DecodeBestAudioStream(filter);
				audioFile->PreparePipeline();
				while (!audioFile->IsDone())
				{
					audioFile->Step();
				}
				muxer->Close();
				delete muxer;
				delete codec;
				delete encoder;
				delete audioFile;
				delete filter;
				system("CLS");
				name.clear();
				for (int k = 0; k < narrowstr.length() - 4; k++)
				{
					name.push_back(narrowstr[k]);
				}
				str.clear();
				for (int k = 0; k < tempstr.length(); k++)
				{
					str.push_back(tempstr[k]);
				}*/
			}
			/*else
			{

			}*/
			for (int i = str.length() - 1; i >= 0; i--) // erase path part of str
			{
				if (str[i] == '\\')
				{
					str.erase(str.begin(), str.begin() + i + 1);
					break;
				}
			}
			SetWindowTextW(GetConsoleWindow(), name.c_str()); // set window title to name
			std::clock_t start = clock(); // starting time
			narrowstr.clear();
			for (int j = 0; j < str.length(); j++) // narrowstr is the non wstring version of str
			{
				narrowstr.push_back(str[j]);
			}
			ffmpegcpp::ContainerInfo info;
			try
			{
				ffmpegcpp::Demuxer* demuxer = new ffmpegcpp::Demuxer(narrowstr.c_str());
				info = demuxer->GetInfo(); // get total length of song
			}
			catch (ffmpegcpp::FFmpegException e) // if it fails (will fail if song has unicode)
			{
				SetCurrentDirectoryA(exepath.c_str()); // set location to where exe file is, instead of music
				std::ofstream fout;
				fout.open("errors.log", std::ofstream::app); // log error
				time_t timething;
				struct tm* timeinfo;
				time(&timething);
				timeinfo = localtime(&timething);
				fout << '[' << timeinfo->tm_mday << '-' << timeinfo->tm_mon + 1 << '-' << timeinfo->tm_year + 1900 << ' ' << timeinfo->tm_hour << ':' << timeinfo->tm_min << ':' << timeinfo->tm_sec << "] FFmpeg Exception: " << e.what() << ";    Song: " << narrowstr << std::endl;
				fout.close();
				SetCurrentDirectoryW(path.c_str());
				next = -1;
				continue; // next song
			}
			int error = mciSendStringW((std::wstring(L"open \"") + str + std::wstring(L"\" alias CURR_SND")).c_str(), NULL, 0, 0); // open song
			if (error != 0) // if mcisendstring has error
			{
				SetCurrentDirectoryA(exepath.c_str());
				std::ofstream fout;
				fout.open("errors.log", std::ofstream::app); // log error
				time_t timething;
				struct tm* timeinfo;
				time(&timething);
				timeinfo = localtime(&timething);
				fout << '[' << timeinfo->tm_mday << '-' << timeinfo->tm_mon + 1 << '-' << timeinfo->tm_year + 1900 << ' ' << timeinfo->tm_hour << ':' << timeinfo->tm_min << ':' << timeinfo->tm_sec << "] Mcisendstring Error: " << error << ";    Song: " << narrowstr << std::endl;
				fout.close();
				SetCurrentDirectoryW(path.c_str());
				next = -1;
				continue; // next song
			}
			location = updatedisplay("null", getcurrentlocation("pauseplay"), name, 0, true, false, 0, info.durationInSeconds); // update the console ui
			nowplaying = name;
			mciSendStringA("play CURR_SND", NULL, 0, 0); // play the song
			mciSendStringA(("setaudio CURR_SND volume to " + std::string(volume)).c_str(), NULL, 0, 0); // set volume
			thyme = clock();
			lastbar = 0;
			lastsec = 0;
			while (true)
			{
				Sleep(10);
				GetAsyncKeyState(176);
				GetAsyncKeyState(177);
				GetAsyncKeyState(179);
				GetAsyncKeyState(32);
				clock_t current = clock();
				if (((current - start) / 1000.0) / info.durationInSeconds * 20 > lastbar) // update bar count
				{
					lastbar = ((current - start) / 1000) / info.durationInSeconds * 20;
					location = updatedisplay("null", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
				}
				if (current - start >= info.durationInSeconds * 1000) // if song is over
				{
					break;
				}
				if ((current - start) / 1000 > lastsec) // update second count
				{
					lastsec = (current - start) / 1000;
					location = updatedisplay("null", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
				}
				if (GetAsyncKeyState(179) || (GetAsyncKeyState(32) && GetForegroundWindow() == GetConsoleWindow())) // if paused
				{
					mciSendString("pause CURR_SND", NULL, 0, 0); // pause
					clock_t current = clock(); // get paused time
					SetConsoleTitleW(L"PAUSED"); // set window name to PAUSED
					location = updatedisplay("null", getcurrentlocation(location), name, lastbar, true, true, lastsec, info.durationInSeconds); // update console ui to paused
					paused = true;
					timesincestart = current - start;
					while (GetAsyncKeyState(179) || GetAsyncKeyState(32)) // while the key is held
					{
						Sleep(1);
					}
					while (!GetAsyncKeyState(179) && !(GetAsyncKeyState(32) && GetForegroundWindow() == GetConsoleWindow())) // until user unpauses
					{
						Sleep(1);
						if (GetAsyncKeyState(VK_LEFT) && GetForegroundWindow() == GetConsoleWindow()) // check for left arrow key
						{
							while (GetAsyncKeyState(VK_LEFT))
							{
								Sleep(1);
							}
							location = updatedisplay("left", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds); // move "cursor" left
						}
						if (GetAsyncKeyState(VK_RIGHT) && GetForegroundWindow() == GetConsoleWindow())
						{
							while (GetAsyncKeyState(VK_RIGHT))
							{
								Sleep(1);
							}
							location = updatedisplay("right", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
						}
						if (GetAsyncKeyState(VK_UP) && GetForegroundWindow() == GetConsoleWindow())
						{
							while (GetAsyncKeyState(VK_UP))
							{
								Sleep(1);
							}
							location = updatedisplay("up", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
						}
						if (GetAsyncKeyState(VK_DOWN) && GetForegroundWindow() == GetConsoleWindow())
						{
							while (GetAsyncKeyState(VK_DOWN))
							{
								Sleep(1);
							}
							location = updatedisplay("down", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
						}
					}
					while (GetAsyncKeyState(179) || GetAsyncKeyState(32)) // while key is held
					{
						Sleep(1);
					}
					start = clock() - current + start; // set the start time to account for paused time
					mciSendStringW(L"play CURR_SND", NULL, 0, 0); // resume the song
					SetConsoleTitleW(name.c_str()); // set window name
					location = updatedisplay("null", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds); // update to unpause in console ui
					paused = false;
				}
				if (GetAsyncKeyState(177)) // previous song
				{
					mciSendString("close CURR_SND", NULL, 0, 0);
					while (GetAsyncKeyState(177))
					{
						Sleep(1);
					}
					next = -2;
					break;
				}
				if (GetAsyncKeyState(176)) // next song
				{
					mciSendString("close CURR_SND", NULL, 0, 0);
					while (GetAsyncKeyState(176))
					{
						Sleep(1);
					}
					break;
				}
				if (GetAsyncKeyState(VK_LEFT) && GetForegroundWindow() == GetConsoleWindow()) // if left arrow key is pressed
				{
					while (GetAsyncKeyState(VK_LEFT))
					{
						Sleep(1);
					}
					location = updatedisplay("left", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds); // move "cursor" left
				}
				if (GetAsyncKeyState(VK_RIGHT) && GetForegroundWindow() == GetConsoleWindow())
				{
					while (GetAsyncKeyState(VK_RIGHT))
					{
						Sleep(1);
					}
					location = updatedisplay("right", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
				}
				if (GetAsyncKeyState(VK_UP) && GetForegroundWindow() == GetConsoleWindow())
				{
					while (GetAsyncKeyState(VK_UP))
					{
						Sleep(1);
					}
					location = updatedisplay("up", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
				}
				if (GetAsyncKeyState(VK_DOWN) && GetForegroundWindow() == GetConsoleWindow())
				{
					while (GetAsyncKeyState(VK_DOWN))
					{
						Sleep(1);
					}
					location = updatedisplay("down", getcurrentlocation(location), name, lastbar, true, false, lastsec, info.durationInSeconds);
				}
			}
			mciSendString("close CURR_SND", NULL, 0, 0);
			if (next != -2) // if not previous song, set next to normal
			{
				next = -1;
			}
		}
	}
}