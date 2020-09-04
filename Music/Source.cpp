#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

#pragma comment(lib, "Winmm.lib")
#define vector std::vector
#define string std::string

void shuffle(vector<int>& curlist, int n)
{
	int last, last2, last3, temp;
	vector<int> used;
	last = curlist[n - 1];
	last2 = curlist[n - 2];
	last3 = curlist[n - 3];
	used.push_back(last);
	used.push_back(last2);
	used.push_back(last3);
	for (int i = 0; i < n; i++)
	{
		curlist[i] = -1;
	}
	srand(time(NULL));
	curlist[rand() % (n - 4)] = last;
	temp = rand() % (n - 4);
	while (curlist[temp] != -1)
	{
		srand(time(NULL));
		temp = rand() % (n - 4);
	}
	curlist[temp] = last2;
	temp = rand() % (n - 4);
	while (curlist[temp] != -1)
	{
		srand(time(NULL));
		temp = rand() % (n - 4);
	}
	curlist[temp] = last3;
	for (int i = 0; i < n; i++)
	{
		sort(used.begin(), used.end());
		if (binary_search(used.begin(), used.end(), i))
		{
			continue;
		}
		temp = rand() % n;
		while (curlist[temp] != -1)
		{
			temp = rand() % n;
		}
		curlist[temp] = i;
		used.push_back(i);
	}
}

int main()
{
	SetConsoleTitle("Music");
	ShowWindow(GetConsoleWindow(), SW_SHOW);
	std::ofstream fout("hwnd");
	fout << int(GetConsoleWindow()) << std::endl;
	fout.close();
	int n = 0, next = -1, ind = -1, thyme;
	const char* c;
	string path, str, temp, name;
	vector<int> curlist, list;
	vector<string> v;
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = KEYEVENTF_KEYUP;
	input.ki.time = 0;
	input.ki.wVk = 7;
	std::ifstream fin("path");
	getline(fin, path);
	fin.close();
	if (path == "")
	{
		std::cout << "What folder are your songs located in?" << std::endl;
		getline(std::cin, path);
		fout.open("path");
		fout << path << std::endl;
		fout.close();
		system("CLS");
	}
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		str = entry.path().u8string();
		str.insert(0, "\'");
		str += "\'";
		v.push_back(str);
		n++;
		curlist.push_back(n - 1);
	}
	thyme = clock();
	system("start ssmhelper.exe");
	while (true)
	{
		shuffle(curlist, n);
		next = -1;
		for (int i = 0; i < n; i++)
		{
			if (next == -1)
			{
				if (i != (ind + 1) % n)
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
			else if (next == 1)
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
			str = "powershell Add-Type -AssemblyName presentationCore; $wmplayer = New-Object System.Windows.Media.MediaPlayer; $wmplayer.Open(";
			/*if (!(v[next][v[next].length() - 5] == '.' && v[next][v[next].length() - 4] == 'w' && v[next][v[next].length() - 3] == 'a' && v[next][v[next].length() - 2] == 'v'))
			{
				next = -1;
				continue;
			}*/
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
			}
		}
	}
}