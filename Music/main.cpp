#include "discord/discord.h"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
using namespace std; // Whatever
#pragma comment(lib, "discord_game_sdk.dll.lib")

struct DiscordState {
	discord::User currentUser;
	unique_ptr<discord::Core> core;
};

//struct DiscordRichPresence {
//	const char* state;
//	const char* details;
//	int64_t startTimestamp;
//	int64_t endTimestamp;
//	const char* largeImageKey; /* max 32 bytes */
//	const char* largeImageText; /* max 128 bytes */
//	const char* smallImageKey; /* max 32 bytes */
//	const char* smallImageText; /* max 128 bytes */
//	const char* partyId; /* max 128 bytes */
//	int partySize;
//	int partyMax;
//	const char* matchSecret; /* max 128 bytes */
//	const char* joinSecret; /* max 128 bytes */
//	const char* spectateSecret; /* max 128 bytes */
//	int8_t instance;
//};

void threadJob() {
	DiscordState state{};
	discord::Core* core{};
	auto resp = discord::Core::Create(792202762008002580, DiscordCreateFlags_Default, &core);
	state.core.reset(core);
	discord::Activity activity{};
	//activity.SetName("[Custom Name for] Discord-Playing-Changer");
	activity.SetDetails("Cool!");
	activity.SetState("Chilling");
	activity.SetType(discord::ActivityType::Playing);
	discord::ActivityTimestamps tmstp{};
	tmstp.SetStart(time(NULL));
	activity.GetTimestamps() = tmstp;
	core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
		cout << ((result == discord::Result::Ok) ? "Success: " : "Failed: ") << "Setting user 'playing' activity." << endl;
		});

	/*DiscordRichPresence dpr{};
	dpr.details = "Testing";
	dpr.startTimestamp = time(NULL);
	dpr.largeImageKey = "ico-contrast";*/

	while (!!1) {
		this_thread::sleep_for(chrono::milliseconds(16));
		state.core->RunCallbacks();
	}
}

int main() {
	thread t = thread(threadJob);
	t.detach();
	this_thread::sleep_for(chrono::minutes(1));
}