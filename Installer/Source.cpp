#include <conio.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <Windows.h>
#include <ObjIdl.h>
#include <ShlObj.h>
#include <ShObjIdl_core.h>
#include "resource.h"

void outputfiles()
{
	HMODULE hmod = GetModuleHandle(NULL);
	HRSRC res;
	res = FindResource(hmod, MAKEINTRESOURCE(IDR_EXECUTABLE1), TEXT("executable"));
	DWORD size = SizeofResource(hmod, res);
	HGLOBAL hmemory = LoadResource(hmod, res);
	LPVOID data = LockResource(hmemory);
	std::ofstream fout("7za.exe", std::ios::binary);
	fout.write((char*)(data), size);
	fout.close();

	res = FindResource(hmod, MAKEINTRESOURCE(IDR_ARCHIVE1), TEXT("archive"));
	size = SizeofResource(hmod, res);
	hmemory = LoadResource(hmod, res);
	data = LockResource(hmemory);
	fout.open("bin.7z", std::ios::binary);
	fout.write((char*)(data), size);
	fout.close();
}

int main()
{
	bool startmenu, desktop;
	std::wstring path;
start:
	std::cout << "Enter installation path (or press enter for Default)" << std::endl;
	getline(std::wcin, path);
	if (path == L"")
	{
		PWSTR* pwstr = new PWSTR;
		SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, NULL, NULL, pwstr);
		path = std::wstring(*pwstr) + L"\\Hakr-Netwurk\\Music\\";
	}
	if (path.back() != '/' && path.back() != '\\')
	{
		path += '\\';
	}
	try
	{
		std::filesystem::create_directories(path);
	}
	catch (std::filesystem::filesystem_error e)
	{
		system("CLS");
		std::cout << e.what() << std::endl;
		goto start;
	}
	system("CLS");
	std::cout << "Add start menu shortcut? (Y/N or enter for Y)" << std::endl;
startmenu:
	char c = _getch();
	if (c == 'y' || c == '\r')
	{
		startmenu = true;
	}
	else if (c == 'n')
	{
		startmenu = false;
	}
	else
	{
		goto startmenu;
	}
	system("CLS");
	std::cout << "Add desktop shortcut? (Y/N or enter for Y)" << std::endl;
desktop:
	c = _getch();
	if (c == 'y' || c == '\r')
	{
		desktop = true;
	}
	else if (c == 'n')
	{
		desktop = false;
	}
	else
	{
		goto desktop;
	}
	system("CLS");
	IShellLinkW* shortcut;
	IPersistFile* persistfile;
	CoInitialize(NULL);
	CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)(&shortcut));
	shortcut->SetPath((path + L"Music.exe").c_str());
	shortcut->SetWorkingDirectory(path.c_str());
	shortcut->QueryInterface(IID_IPersistFile, (LPVOID*)(&persistfile));
	if (startmenu)
	{
		PWSTR* pwstr = new PWSTR;
		SHGetKnownFolderPath(FOLDERID_Programs, NULL, NULL, pwstr);
		persistfile->Save((std::wstring(*pwstr) + L"\\Hakr-Netwurk\\Music.lnk").c_str(), TRUE);
	}
	if (desktop)
	{
		PWSTR* pwstr = new PWSTR;
		SHGetKnownFolderPath(FOLDERID_Desktop, NULL, NULL, pwstr);
		persistfile->Save((std::wstring(*pwstr) + L"\\Music.lnk").c_str(), TRUE);
	}
	SetCurrentDirectoryW(path.c_str());
	outputfiles();
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	std::cout << "Extracting files" << std::endl;
	CreateProcessW(L"7za.exe", (wchar_t*)("7za.exe e -y bin.7z"), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);
	std::filesystem::remove("7za.exe");
	std::filesystem::remove("bin.7z");
	std::cout << "Done!" << std::endl;
}