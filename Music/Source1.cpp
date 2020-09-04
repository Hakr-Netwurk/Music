#include <fstream>
#include <iostream>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	char mesg[128];

	switch (CEvent)
	{
	case CTRL_C_EVENT:
		break;
	case CTRL_BREAK_EVENT:
		break;
	case CTRL_CLOSE_EVENT:
		break;
	}
	system("taskkill /f /im music.exe");
	system("taskkill /f /im powershell.exe");
	return TRUE;
}

void Suspend(PCSTR name)
{
	DWORD pid = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	PROCESSENTRY32 process;
	THREADENTRY32 thread;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	thread.dwSize = sizeof(THREADENTRY32);
	if (Process32First(snapshot, &process))
	{
		do
		{
			if (std::string(process.szExeFile) == std::string(name))
			{
				pid = process.th32ProcessID;
				Thread32First(hThreadSnapshot, &thread);
				do
				{
					if (thread.th32OwnerProcessID == pid)
					{
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, thread.th32ThreadID);
						SuspendThread(hThread);
						CloseHandle(hThread);
					}
				} while (Thread32Next(hThreadSnapshot, &thread));
				CloseHandle(hThreadSnapshot);
			}
		} while (Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
}

void Resume(PCSTR name)
{
	DWORD pid = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	PROCESSENTRY32 process;
	THREADENTRY32 thread;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);
	thread.dwSize = sizeof(THREADENTRY32);
	if (Process32First(snapshot, &process))
	{
		do
		{
			if (std::string(process.szExeFile) == std::string(name))
			{
				pid = process.th32ProcessID;
				Thread32First(hThreadSnapshot, &thread);
				do
				{
					if (thread.th32OwnerProcessID == pid)
					{
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, thread.th32ThreadID);
						ResumeThread(hThread);
						CloseHandle(hThread);
					}
				} while (Thread32Next(hThreadSnapshot, &thread));
				CloseHandle(hThreadSnapshot);
			}
		} while (Process32Next(snapshot, &process));
	}
	CloseHandle(snapshot);
}

int main()
{
	SetConsoleTitle("Music");
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	int x, y, w, h;
	bool paused = false;
	std::string str;
	HANDLE handle = HANDLE(-1);
	HWND hwnd;
	INPUT input;
	RECT rect;
	input.type = INPUT_KEYBOARD;
	input.ki.time = 0;
	input.ki.dwFlags = 0;
	input.ki.wVk = 7;
	std::ifstream fin("hwnd");
	fin >> str;
	fin.close();
	std::ofstream fout("action");
	fout << "-1" << std::endl;
	fout.close();
	hwnd = HWND(stoi(str));
	SetForegroundWindow(hwnd);
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	while (true)
	{
		if (GetAsyncKeyState(176) != 0)
		{
			system("taskkill /f /im powershell.exe");
			while (GetAsyncKeyState(176) != 0)
			{
			}
		}
		if (GetAsyncKeyState(177) != 0)
		{
			SendInput(1, &input, sizeof(INPUT));
			system("taskkill /f /im powershell.exe");
			while (GetAsyncKeyState(177) != 0)
			{
			}
		}
		if (GetAsyncKeyState(179) != 0)
		{
			if (paused)
			{
				Resume("powershell.exe");
				system("CLS");
				//ShowWindow(hwnd, SW_SHOW);
				GetWindowRect(GetConsoleWindow(), &rect);
				x = rect.left;
				y = rect.top;
				w = rect.right - rect.left;
				h = rect.bottom - rect.top;
				SetWindowPos(hwnd, GetConsoleWindow(), x, y, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
				ShowWindow(GetConsoleWindow(), SW_HIDE);
				if (GetForegroundWindow() == GetConsoleWindow())
				{
					SetForegroundWindow(hwnd);
				}
			}
			else
			{
				Suspend("powershell.exe");
				system("CLS");
				std::ifstream fin("cursong");
				getline(fin, str);
				fin.close();
				GetWindowRect(hwnd, &rect);
				x = rect.left;
				y = rect.top;
				w = rect.right - rect.left;
				h = rect.bottom - rect.top;
				std::cout << "PAUSED: " << str << std::endl;
				ShowWindow(hwnd, SW_HIDE);
				//ShowWindow(GetConsoleWindow(), SW_SHOW);
				SetWindowPos(GetConsoleWindow(), hwnd, x, y, w, h, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
				if (GetForegroundWindow() == hwnd)
				{
					SetForegroundWindow(GetConsoleWindow());
				}
			}
			paused = !paused;
			while (GetAsyncKeyState(179) != 0)
			{
			}
		}
		if ((GetForegroundWindow() == hwnd || GetForegroundWindow() == GetConsoleWindow()) && GetAsyncKeyState(32) != 0)
		{
			if (paused)
			{
				Resume("powershell.exe");
				system("CLS");
				//ShowWindow(hwnd, SW_SHOW);
				GetWindowRect(GetConsoleWindow(), &rect);
				x = rect.left;
				y = rect.top;
				w = rect.right - rect.left;
				h = rect.bottom - rect.top;
				SetWindowPos(hwnd, GetConsoleWindow(), x, y, w, h, SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
				ShowWindow(GetConsoleWindow(), SW_HIDE);
				SetForegroundWindow(hwnd);
			}
			else
			{
				Suspend("powershell.exe");
				system("CLS");
				std::ifstream fin("cursong");
				getline(fin, str);
				fin.close();
				GetWindowRect(hwnd, &rect);
				x = rect.left;
				y = rect.top;
				w = rect.right - rect.left;
				h = rect.bottom - rect.top;
				std::cout << "PAUSED: " << str << std::endl;
				ShowWindow(hwnd, SW_HIDE);
				//ShowWindow(GetConsoleWindow(), SW_SHOW);
				SetWindowPos(GetConsoleWindow(), hwnd, x, y, w, h, SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
				SetForegroundWindow(GetConsoleWindow());
			}
			paused = !paused;
			while (GetAsyncKeyState(32) != 0)
			{
			}
		}
		if (!IsWindow(hwnd))
		{
			return 0;
		}
		Sleep(32);
	}
}