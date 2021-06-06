#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
bool isvolume = false;
bool ishelp = false; // If I am ON the help screen (or need to be ON it)
bool helpPainted = false; // Additional bool to prevent infinite repaint of help screen (flashing, ugh)
extern int next = -1, foldernum = -1;
extern std::vector<int> volumes = {};
extern std::wstring name = L"", path = L"";
extern std::string exepath = "";

int colortonum(std::string str) // translate color string to a number
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

void color(std::string text, std::string background) // set console text colors accordingly
{
	SetConsoleTextAttribute(console, colortonum(text) + colortonum(background) * 16);
}

std::string formattime(int timestamp) // formats the time to a readable dd:hh:mm:ss
{
	std::string seconds, minutes, hours, days, finalstr;
	seconds = std::to_string(timestamp % 60);
	timestamp /= 60;
	minutes = std::to_string(timestamp % 60);
	timestamp /= 60;
	hours = std::to_string(timestamp % 60);
	timestamp /= 60;
	days = std::to_string(timestamp);
	while (seconds.length() < 2)
	{
		seconds.insert(seconds.begin(), '0');
	}
	while (minutes.length() < 2)
	{
		minutes.insert(minutes.begin(), '0');
	}
	while (hours.length() < 2 && hours != "0")
	{
		hours.insert(hours.begin(), '0');
	}
	if (days != "0")
	{
		finalstr += days + ":";
	}
	if (hours != "0")
	{
		finalstr += hours + ":";
	}
	finalstr += minutes + ":" + seconds;
	return finalstr;
}

std::string wtomb(std::wstring wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

std::pair<int, int> getcurrentlocation(std::string str) // get location of string (eg "pauseplay")
{
	if (str == "menu")
	{
		return std::make_pair(1, 1);
	}
	if (str == "progbar")
	{
		return std::make_pair(2, 3);
	}
	if (str == "help")
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
		return std::make_pair(3, 5);
	}
	// PATCH: NOP for help
	if (str == "nop") {
		return std::make_pair(-1, -1);
	}
}

/*PROTOTYPE LAYOUT

≡

C418 - Alpha
[-                   ] 01:00/10:50
_    <<   |>   >>    ↔
Volume <---------------o---->

*/

std::string updatedisplay(std::string action, std::pair<int, int> location, std::wstring name, int numbars, bool autosaveon, bool paused, int elapsed, int total) // updates console text (duh)
{
	CONSOLE_SCREEN_BUFFER_INFO screen;
	GetConsoleScreenBufferInfo(console, &screen);
	std::string selected, narrowname = std::string(name.begin(), name.end()), temp;
	std::vector<std::vector<std::string>> v = // lookup table for different things
	{
		{ "null", "null", "null" },
		{ "null", "menu", "menu", "menu", "null" },
		{ "null", "progbar", "progbar", "progbar", "progbar", "progbar", "null" },
		{ "null", "help", "prev", "pauseplay", "next", "volume", "null" },
		{ "null", "null", "null", "null", "null", "null", "null" }
	};
	if (location == std::pair(-1, -1)) {
		// Just return "nop" again. Hang here!
		return "nop";
	}
	// position selected accordingly
	if (action == "up")
	{
		selected = v[location.first - 1][location.second];
	}
	else if (action == "down")
	{
		selected = v[location.first + 1][location.second];
	}
	else if (action == "left")
	{
		selected = v[location.first][location.second - 1];
	}
	else if (action == "right")
	{
		selected = v[location.first][location.second + 1];
	}
	else if (action == "volume")
	{
		selected = "pauseplay";
		isvolume = true;
	}
	else if (action == "help") {
		ishelp = true;
	}

	if (action == "back") // reset ui
	{
		selected = "pauseplay";
		isvolume = false;
		// This is for if we came back from the help screen.
		if (ishelp) {
			system("CLS");
			ishelp = false;
			helpPainted = false;
		}
	}
	// for out of bounds, or updatedisplay without user input
	if (action == "null" || selected == "null")
	{
		selected = v[location.first][location.second];
	}
	if (isvolume && (selected == "prev" || selected == "next"))
	{
		SetCurrentDirectoryA((exepath + '\\' + std::to_string(foldernum)).c_str());
		int last = -1;
		std::vector<std::string> v;
		std::ifstream fin("volume", std::ios::app);
		while (fin)
		{
			getline(fin, temp);
			v.push_back(temp);
		}
		fin.close();
		for (int i = 0; i < v.size(); i++)
		{
			if (v[i] == wtomb(name))
			{
				last = i;
			}
		}
		for (int i = v.size() - 1; i >= 1; i--)
		{
			if (v[i - 1] != "")
			{
				break;
			}
			v.erase(v.end() - 1);
		}
		if (selected == "prev") // decrease volume (prev is left of pauseplay/volume)
		{
			volumes[next] = max(0, min(100, 5 * (round(volumes[next] / 5.0)) - 5));
			selected = "pauseplay";
			mciSendStringA(("setaudio CURR_SND volume to " + std::to_string(volumes[next] * 10)).c_str(), NULL, 0, 0); // set volume
		}
		else if (selected == "next") // increase volume
		{
			volumes[next] = max(0, min(100, 5 * (round(volumes[next] / 5.0)) + 5));
			selected = "pauseplay";
			mciSendStringA(("setaudio CURR_SND volume to " + std::to_string(volumes[next] * 10)).c_str(), NULL, 0, 0); // set volume
		}
		if (last == -1)
		{
			std::ofstream fout("volume", std::ostream::app);
			fout << std::endl << wtomb(name) << std::endl << volumes[next] << std::endl;
			fout.close();
		}
		else
		{
			v[last + 1] = std::to_string(volumes[next]);
			std::ofstream fout("volume");
			for (int i = 0; i < v.size(); i++)
			{
				fout << v[i] << std::endl;
			}
			fout.close();
		}
		SetCurrentDirectoryW(path.c_str());
	}

	// Help dialog painting
	if (ishelp && !helpPainted) {
		system("CLS");
		// Get ready for... ShItTy cOdE!
		std::cout << "----------------------------------------------------" << std::endl
			<< "|                   Music: Help                    |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| I. Introduction                                  |" << std::endl
			<< "|                                                  |" << std::endl
			<< "|  supsm/music is a lightweight, fast CLI-based mu-|" << std::endl
			<< "| sic player. It contains a relatively simple TUI, |" << std::endl
			<< "| used as the main control interface.              |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| II. The Interface                                |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| " << char(240) << " (1)                                            |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| Title of Song File                               |" << std::endl
			<< "| [                    ] 00:00/04:00  (2)          |" << std::endl
			<< "| ? (3)    << (4)   |> (5)   >> (6)    ↔ (7)       |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| (1): Menu icon (does nothing atm)                |" << std::endl
			<< "| (2): Progress bar. It will fill up with dashes as|" << std::endl
			<< "| you progress through the song.                   |" << std::endl
			<< "| (3): Help. Select it to bring up this dia-       |" << std::endl
			<< "| log box. (Not to be confused with a cry for help)|" << std::endl
			<< "| (4): Previous. (Do I really need to explain??)   |" << std::endl
			<< "| (5): Play/Pause. Go figure! [SPACE]              |" << std::endl
			<< "| (6): Huh... I wonder what it could be...?        |" << std::endl
			<< "| (7): Additional options. Currently serves to adj-|" << std::endl
			<< "| ust volume. With the volume slider selected, use |" << std::endl
			<< "| the arrow keys. Press *ESC* when you're done,    |" << std::endl
			<< "| it's a bug.                                      |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| Select things using the arrow keys. Hit enter to |" << std::endl
			<< "| \"activate\" them.                                 |" << std::endl // IDK why, there's a strange gap here that requires extra space
			<< "|                                                  |" << std::endl
			<< "| III. General Usage                               |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| Upon first boot, or when the \"path\" file is del- |" << std::endl // Here too
			<< "|  eted, the program will prompt you for your music|" << std::endl
			<< "| location. Afterwards, refer to the above section |" << std::endl
			<< "| to naviagte the interface.                       |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| IV. Misc                                         |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| Music has Discord integration! (But this is curr-|" << std::endl
			<< "| ently relatively basic. Development will continu-|" << std::endl
			<< "| e in the future!                                 |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| V. Credits                                       |" << std::endl
			<< "|                                                  |" << std::endl
			<< "| The following people helped make Music:          |" << std::endl
			<< "| - supsm (main dev)                               |" << std::endl
			<< "| - Vbbab (helper & secondary dev)                 |" << std::endl
			<< "----------------------------------------------------" << std::endl
			<< "                                                    " << std::endl
			<< "                [ E S C ] Go back                   " << std::endl;
		helpPainted = true;
		return "nop"; // :)
	}
	if (!ishelp) { // Don't paint the main player if the help dialog is painted
		SetConsoleCursorPosition(console, { 0, 0 });
		color("light gray", "black");
		if (selected == "menu") // if the selected is the menu, invert the colors to show it's selected
		{
			color("black", "light gray");
		}
		std::cout << char(240); // menu character
		color("light gray", "black");
		std::cout << std::endl << std::endl << narrowname << std::endl;
		if (selected == "progbar")
		{
			color("black", "light gray");
		}
		std::cout << '[';
		for (int i = 0; i < numbars; i++)
		{
			std::cout << '-';
		}
		for (int i = numbars; i < 20; i++) // fill the rest with space
		{
			std::cout << ' ';
		}
		std::cout << ']';
		std::cout << ' ' << formattime(elapsed) << '/' << formattime(total);
		color("light gray", "black");
		std::cout << std::endl;
		if (!isvolume)
		{
			if (selected == "help")
			{
				color("black", "light gray");
			}
			std::cout << "?";
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
		else
		{
			std::cout << "Volume ";
			if (selected == "pauseplay")
			{
				color("black", "light gray");
			}
			std::cout << '<';
			for (int i = 1; i < volumes[next] / 5; i++)
			{
				std::cout << '-';
			}
			std::cout << 'o';
			for (int i = volumes[next] / 5; i < 20; i++)
			{
				std::cout << '-';
			}
			std::cout << '>';
		}
		color("light gray", "black");
		std::cout << "       ";
		CONSOLE_CURSOR_INFO cursorinfo;
		GetConsoleCursorInfo(console, &cursorinfo);
		cursorinfo.bVisible = false; // make cursor invisible so it doesnt flash
		SetConsoleCursorInfo(console, &cursorinfo);
		color("light gray", "black"); // reset text color
		return selected; // return the new selected
	}
	else {
		return "pauseplay"; // ignored?
	}
}