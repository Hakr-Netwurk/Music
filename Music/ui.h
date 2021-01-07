#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

void clear() {
	COORD topLeft = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;
	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

int colortonum(std::string str)
{
	std::vector<std::string> v = { "black", "dark blue", "dark green", "dark cyan", "dark red", "dark magenta", "dark yellow", "light gray", "dark gray", "blue", "green", "cyan", "red", "magenta", "yellow", "white" };
	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == str)
		{
			return i;
		}
	}
	return -1;
}

void color(std::string text, std::string background)
{
	SetConsoleTextAttribute(console, colortonum(text) + colortonum(background) * 16);
}

std::pair<int, int> getcurrentlocation(std::string str)
{
	if (str == "menu")
	{
		return std::make_pair(1, 1);
	}
	if (str == "progbar")
	{
		return std::make_pair(2, 1);
	}
	if (str == "autosave")
	{
		return std::make_pair(3, 1);
	}
	if (str == "prev")
	{
		return std::make_pair(3, 2);
	}
	if (str == "pauseplay")
	{
		return std::make_pair(3, 3);
	}
	if (str == "next")
	{
		return std::make_pair(3, 4);
	}
	if (str == "volume")
	{
		return std::make_pair(4, 5);
	}
}

/*PROTOTYPE LAYOUT
/
≡

[--------------------]
_    <<   |>   >>    ↔
*
*/

void updatedisplay(std::string action, std::pair<int, int> location, std::wstring name, int elapsed, int total, bool autosaveon, bool paused)
{
	CONSOLE_SCREEN_BUFFER_INFO screen;
	GetConsoleScreenBufferInfo(console, &screen);
	std::string selected;
	std::vector<std::vector<std::string>> v =
	{
		{ "null", "null", "null" },
		{ "null", "menu", "null" },
		{ "null", "progbar", "progbar", "progbar", "progbar", "progbar", "null" },
		{ "null", "autosave", "prev", "pauseplay", "next", "volume", "null" },
		{ "null", "null", "null", "null", "null", "null", "null" }
	};
	if (action == "up")
	{
		selected = v[location.first - 1][location.second];
	}
	if (action == "down")
	{
		selected = v[location.first + 1][location.second];
	}
	if (action == "left")
	{
		selected = v[location.first][location.second - 1];
	}
	if (action == "right")
	{
		selected = v[location.first][location.second + 1];
	}
	if (selected == "null")
	{
		selected = v[location.first][location.second];
	}
	clear();
	color("light gray", "black");
	if (selected == "menu")
	{
		color("black", "light gray");
	}
	std::cout << char(240);
	color("light gray", "black");
	std::cout << std::endl << std::endl;
	if (selected == "progbar")
	{
		color("black", "light gray");
	}
	std::cout << "[--------------------]";
	color("light gray", "black");
	std::cout << std::endl;
	if (selected == "autosave")
	{
		color("black", "light gray");
	}
	if (autosaveon)
	{
		std::cout << char(251);
	}
	else
	{
		std::cout << '_';
	}
	color("light gray", "black");
	std::cout << "    ";
	if (selected == "prev")
	{
		color("black", "light gray");
	}
	std::cout << "<<";
	color("light gray", "black");
	std::cout << "   ";
	if (selected == "pauseplay")
	{
		color("black", "light gray");
	}
	if (paused)
	{
		std::cout << "|>";
	}
	else
	{
		std::cout << "||";
	}
	color("light gray", "black");
	std::cout << "   ";
	if (selected == "next")
	{
		color("black", "light gray");
	}
	std::cout << ">>";
	color("light gray", "black");
	std::cout << "    ";
	if (selected == "volume")
	{
		color("black", "light gray");
	}
	std::cout << char(29);
}