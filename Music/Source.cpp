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

struct DiscordState
{
	std::unique_ptr<discord::Core> core;
};

std::set<HANDLE> suspendedthreads;
std::thread t, t2, t3;
std::wstring name;
int threadid, mainthreadid, next = -1;
bool suspended = false, doneplaying, discordstarted = false, idle = false;
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

void play(std::wstring filepath)
{
	if (!discordstarted)
	{
		if (getdiscord())
		{
			startdiscord();
		}
	}
	PlaySoundW(filepath.c_str(), NULL, SND_SYNC);
	doneplaying = true;
}

void waitforkey()
{
	while (true)
	{
		GetAsyncKeyState(176);
		GetAsyncKeyState(177);
		GetAsyncKeyState(179);
		if (GetAsyncKeyState(176) != 0)
		{
			//
		}
		if (GetAsyncKeyState(177) != 0)
		{
			//
		}
		if (GetAsyncKeyState(179) != 0)
		{
			while (GetAsyncKeyState(179) != 0);
			/*HANDLE h = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadid);
			if (!suspended)
			{
				SuspendThread(h);
			}
			else
			{
				ResumeThread(h);
			}*/
			HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			THREADENTRY32 thread;
			thread.dwSize = sizeof(THREADENTRY32);
			Thread32First(hThreadSnapshot, &thread);
			bool tempsus = suspended;
			suspendedthreads.clear();
			do
			{
				if (thread.th32OwnerProcessID == GetCurrentProcessId() && thread.th32ThreadID != GetCurrentThreadId() && thread.th32ThreadID != threadid && thread.th32ThreadID != mainthreadid)
				{
					HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, thread.th32ThreadID);
					if (!suspended)
					{
						SuspendThread(hThread);
						tempsus = true;
						suspendedthreads.insert(hThread);
						SetConsoleTitleW(L"PAUSED");
						system("CLS");
						std::string s(name.begin(), name.end());
						std::cout << "PAUSED: " << s << std::endl;
					}
					else if (suspendedthreads.find(hThread) == suspendedthreads.end())
					{
						ResumeThread(hThread);
						tempsus = false;
						SetConsoleTitleW(name.c_str());
						system("CLS");
						std::string s(name.begin(), name.end());
						std::cout << "Now Playing: " << s << std::endl;
					}
					CloseHandle(hThread);
				}
			} while (Thread32Next(hThreadSnapshot, &thread));
			CloseHandle(hThreadSnapshot);
			suspended = tempsus;
		}
		Sleep(1);
		if (doneplaying)
		{
			return;
		}
	}
}

void deleteused(std::wstring filepath)
{
	if (filepath != L"")
	{
		Sleep(1000);
		DeleteFileW(filepath.c_str());
	}
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
		std::cout << "What folder are your songs located in?" << std::endl;
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
		//str.insert(0, "\'");
		//str += "\'";
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
	//system("start ssmhelper.exe");
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
			if (!(str[str.length() - 1] == 'v' && str[str.length() - 2] == 'a' && str[str.length() - 3] == 'w' && str[str.length() - 4] == '.'))
			{
				if (str[str.length() - 1] == '3' && str[str.length() - 2] == 'p' && str[str.length() - 3] == 'm' && str[str.length() - 4] == '.')
				{
					for (int j = 0; j < name.length(); j++)
					{
						narrowstr.push_back(name[j]);
					}
					narrowstr += ".mp3";
					tempstr = "[ssmtemp]";
					tempstr += narrowstr;
					tempstr.erase(tempstr.end() - 4, tempstr.end());
					tempstr += ".wav";
					ffmpegcpp::Muxer* muxer = new ffmpegcpp::Muxer(tempstr.c_str());
					ffmpegcpp::AudioCodec* codec = new ffmpegcpp::AudioCodec(AV_CODEC_ID_PCM_S16LE);
					ffmpegcpp::AudioEncoder* encoder = new ffmpegcpp::AudioEncoder(codec, muxer);
					ffmpegcpp::Filter* filter = new ffmpegcpp::Filter("volume=0.1", encoder);
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
					system("CLS");
					name.clear();
					for (int j = 0; j < narrowstr.length() - 4; j++)
					{
						name.push_back(narrowstr[j]);
					}
					str.clear();
					for (int j = 0; j < tempstr.length(); j++)
					{
						str.push_back(tempstr[j]);
					}
				}
				else
				{
					continue;
				}
			}
			SetWindowTextW(GetConsoleWindow(), name.c_str());
			std::string s(name.begin(), name.end());
			std::cout << "Now Playing: " << s << std::endl;
			suspended = false;
			doneplaying = false;
			t = std::thread(play, str);
			t2 = std::thread(waitforkey);
			t3 = std::thread(deleteused, str);
			t.join();
			t2.join();
			t3.join();
			if (next != -2)
			{
				next = -1;
			}
			/*str = "powershell Add-Type -AssemblyName presentationCore; $wmplayer = New-Object System.Windows.Media.MediaPlayer; $wmplayer.Open(";
			str += v[next];
			str += "); Start-Sleep 1; $duration = $wmplayer.NaturalDuration.TimeSpan.TotalSeconds; $wmplayer.Volume = 0.1; $wmplayer.Play(); Start-Sleep $duration; $wmplayer.Stop(); $wmplayer.Close()";
			c = str.c_str();
			temp = v[next];
			name.clear();
			for (int i = temp.length() - 2; i >= 0; i--)
			{
				if (temp[i] == '\\')
				{
					break;
				}
				name.insert(name.begin(), temp[i]);
			}
			std::cout << "Now Playing: " << name << std::endl;
			fout.open("cursong");
			fout << name << std::endl;
			fout.close();
			thyme = clock();
			system(c);
			system("CLS");
			next = -1;
			Sleep(32);
			if (GetAsyncKeyState(7) != 0)
			{
				SendInput(1, &input, sizeof(INPUT));
				next = 1;
			}*/
		}
	}
}