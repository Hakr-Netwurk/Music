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

#define volume "volume=0.1"

struct DiscordState
{
	std::unique_ptr<discord::Core> core;
};

std::set<HANDLE> suspendedthreads;
std::thread t, t2, t3;
std::wstring name;
int threadid, mainthreadid, next = -1;
bool suspended = false, doneplaying, discordstarted = false, idle = false;
std::vector<std::string> supportedformats = { "mp3", "ogg", "m4a", "wma", "flac" }; // must be lowercase
DiscordState state{};
discord::Core* core{};
discord::Activity activity{};
discord::ActivityTimestamps timestamp{};

int morerand(int n)
{
	long double cur = long double(rand() % 1000) / 1000;
	n = n % 10000;
	for (int i = 0; i < n; i++)
	{
		cur = abs(4.5864 * cur * (1 - cur));
	}
	return cur * 1000000;
}

void shuffle(std::vector<int>& curlist, int n)
{
	int last, last2, last3, temp;
	std::set<int> used;
	last = curlist[n - 1];
	last2 = curlist[n - 2];
	last3 = curlist[n - 3];
	used.insert(last);
	used.insert(last2);
	used.insert(last3);
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

void discordthing()
{
	threadid = GetCurrentThreadId();
	std::wstring lastname;
	while (true)
	{
		std::string tempstr = "Playing ";
		for (int i = 0; i < name.length(); i++)
		{
			tempstr.push_back(name[i]);
		}
		lastname = name;
		activity.SetDetails(tempstr.c_str());
		timestamp.SetStart(time(NULL));
		activity.GetTimestamps() = timestamp;
		state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {});
		while (true)
		{
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
			if (name != lastname)
			{
				break;
			}
		}
	}
}

void startdiscord()
{
	activity.SetType(discord::ActivityType::Playing);
	auto response = discord::Core::Create(777048406639116309, DiscordCreateFlags_Default, &core);
	state.core.reset(core);
	std::thread discth = std::thread(discordthing);
	discth.detach();
	discordstarted = true;
}

bool getdiscord()
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

int main()
{
	SetConsoleTitle("Music");
	ShowWindow(GetConsoleWindow(), SW_SHOW);
	int n = 0, ind = -1, thyme;
	const char* c;
	std::string temp;
	std::wstring path, str;
	std::vector<int> curlist, list;
	std::vector<std::wstring> v;
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = KEYEVENTF_KEYUP;
	input.ki.time = 0;
	input.ki.wVk = 7;
	std::wifstream fin("path");
	std::wofstream fout;
	getline(fin, path);
	fin.close();
	if (path == L"")
	{
		std::cout << "What folder are your songs located in? [Or put the path in the \"path\" file in the sources dir]" << std::endl;
		std::getline(std::wcin, path);
		fout.open("path");
		fout << path << std::endl;
		fout.close();
		system("CLS");
	}
	SetCurrentDirectoryW(path.c_str());
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		str = entry.path().wstring();
		v.push_back(str);
		n++;
		curlist.push_back(n - 1);
	}
	thyme = clock();
	mainthreadid = GetCurrentThreadId();
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
			if (next == -1)
			{
				/*if (i != (ind + 1) % n)
				{
					next = list[ind];
					i--;
				}
				else
				{*/
				next = curlist[i];
				list.push_back(next);
				//}
				ind++;
			}
			else if (next == -2)
			{
				if (ind > 0)
				{
					ind--;
					if (clock() - thyme >= 3000)
					{
						ind++;
					}
				}
				next = list[ind];
			}
			str = v[next];
			if (str.length() <= 4)
			{
				continue;
			}
			name.clear();
			for (int j = str.length() - 1; j >= 0; j--)
			{
				if (str[j] == '\\')
				{
					break;
				}
				name.insert(name.begin(), str[j]);
			}
			system("CLS");
			name.erase(name.end() - 4, name.end());
			std::string narrowstr, tempstr;
			tempstr.clear();
			narrowstr.clear();
			if (!(str[str.length() - 1] == 'v' && str[str.length() - 2] == 'a' && str[str.length() - 3] == 'w' && str[str.length() - 4] == '.'))
			{
				int format = -1;
				for (int j = 0; j < supportedformats.size(); j++)
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
					if (mismatch)
					{
						continue;
					}
					else
					{
						format = j;
						break;
					}
				}
				if (format == -1)
				{
					continue;
				}
				for (int k = 0; k < name.length(); k++)
				{
					narrowstr.push_back(name[k]);
				}
				narrowstr += ".";
				narrowstr += supportedformats[format];
				tempstr = "[ssmtemp]";
				tempstr += narrowstr;
				tempstr.erase(tempstr.end() - 4, tempstr.end());
				tempstr += ".wav";
				ffmpegcpp::Muxer* muxer = new ffmpegcpp::Muxer(tempstr.c_str());
				ffmpegcpp::AudioCodec* codec = new ffmpegcpp::AudioCodec(AV_CODEC_ID_PCM_S16LE);
				ffmpegcpp::AudioEncoder* encoder = new ffmpegcpp::AudioEncoder(codec, muxer);
				ffmpegcpp::Filter* filter = new ffmpegcpp::Filter(volume, encoder);
				/*int rawAudioSampleRate = 48000;
				int rawAudioChannels = 2;
				ffmpegcpp::RawAudioFileSource* audioFile = new ffmpegcpp::RawAudioFileSource(narrowstr.c_str(), "mp3", rawAudioSampleRate, rawAudioChannels, encoder);*/
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
				}
			}
			SetWindowTextW(GetConsoleWindow(), name.c_str());
			std::string s(name.begin(), name.end());
			std::cout << "Now Playing: " << s << std::endl;
			std::clock_t start = clock();
			narrowstr.clear();
			for (int j = 0; j < str.length(); j++)
			{
				narrowstr.push_back(str[j]);
			}
			ffmpegcpp::Demuxer* demuxer = new ffmpegcpp::Demuxer(narrowstr.c_str());
			ffmpegcpp::ContainerInfo info = demuxer->GetInfo();
			PlaySoundW(str.c_str(), NULL, SND_ASYNC);
			WIN32_FIND_DATAW lpfinddata;
			GetAsyncKeyState(179);
			for (int j = 0; j < info.durationInSeconds * 10; j++)
			{
				Sleep(100);
				if (GetAsyncKeyState(179))
				{
					PlaySoundW(NULL, NULL, SND_ASYNC);
					clock_t current = clock();
					SetConsoleTitleW(L"PAUSED");
					std::string s(name.begin(), name.end());
					std::cout << "PAUSED: " << s << std::endl;
					while (GetAsyncKeyState(179))
					{
						Sleep(1);
					}
					tempstr = std::string(str.begin(), str.end());
					ffmpegcpp::Muxer* muxer = new ffmpegcpp::Muxer(tempstr.c_str());
					ffmpegcpp::AudioCodec* codec = new ffmpegcpp::AudioCodec(AV_CODEC_ID_PCM_S16LE);
					ffmpegcpp::AudioEncoder* encoder = new ffmpegcpp::AudioEncoder(codec, muxer);
					ffmpegcpp::Filter* filter;
					if (name[name.length() - 1] == 'v' && name[name.length() - 2] == 'a' && name[name.length() - 1] == 'w')
					{
						filter = new ffmpegcpp::Filter("volume=1", encoder);
					}
					else
					{
						filter = new ffmpegcpp::Filter(volume, encoder);
					}
					std::string trimfilter = "atrim=start=";
					trimfilter += std::to_string((current - start) / 1000.0);
					ffmpegcpp::Filter* filter2 = new ffmpegcpp::Filter(trimfilter.c_str(), filter);
					ffmpegcpp::Demuxer* audioFile = new ffmpegcpp::Demuxer(narrowstr.c_str());
					audioFile->DecodeBestAudioStream(filter2);
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
					delete filter2;
					while (!GetAsyncKeyState(179))
					{
						Sleep(1);
					}
					while (GetAsyncKeyState(179))
					{
						Sleep(1);
					}
					demuxer = new ffmpegcpp::Demuxer(narrowstr.c_str());
					info = demuxer->GetInfo();
					start = clock() - current + start;
					PlaySoundW(str.c_str(), NULL, SND_ASYNC);
					SetConsoleTitleW(name.c_str());
					s = std::string(name.begin(), name.end());
					std::cout << "Now Playing: " << s << std::endl;
				}
				if (GetAsyncKeyState(177))
				{
					PlaySoundW(NULL, NULL, SND_ASYNC);
					while (GetAsyncKeyState(177))
					{
						Sleep(1);
					}
					break;
				}
			}
			delete demuxer;
			/*suspended = false;
			doneplaying = false;
			t = std::thread(play, str);
			t2 = std::thread(waitforkey);
			t3 = std::thread(deleteused, str);
			t.join();
			t2.join();
			t3.join();*/
			if (next != -2)
			{
				next = -1;
			}
		}
	}
}