#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "discord/discord.h"

//#pragma comment (lib, "discord_game_sdk.dll.lib")

struct DiscordState {
	std::unique_ptr<discord::Core> core;
};

void threadthing()
{
	DiscordState state{};
	discord::Core* core{};
	auto response = discord::Core::Create(792202762008002580, DiscordCreateFlags_Default, &core);
	state.core.reset(core);
	discord::Activity activity{};
	activity.SetDetails("test");
	activity.SetState("test2");
	activity.SetType(discord::ActivityType::Playing);
	discord::ActivityTimestamps timestamp{};
	timestamp.SetStart(time(NULL));
	activity.GetTimestamps() = timestamp;
	state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result)
		{
		std::cout << ((result == discord::Result::Ok) ? "Succeeded" : "Failed") << " updating activity!\n";
		});
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
		state.core->RunCallbacks();
	}
}

int main()
{
	std::thread t = std::thread(threadthing);
	t.detach();
	std::this_thread::sleep_for(std::chrono::minutes(1));
	/*
	std::signal(SIGINT, [](int)
		{
		interrupted = true;
		});
	do
	{
		state.core->RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	} while (!interrupted);*/
}
