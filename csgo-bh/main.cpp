#include "memory.h"
#include <thread>

void trigger_bot();
void radar_hack();
void bunny_hop();
void trigger_handle();
void bunny_handle();

namespace offsets {
	//for bunnyhop
	constexpr auto localPlayer = 0xD8A2CC;
	constexpr auto flags = 0x104;
	constexpr auto forceJump = 0x524CFAC;
	//for trigger
	constexpr auto attack_lmb = 0x31D36E4;
	constexpr auto entityList = 0x4DA31BC;
	constexpr auto crosshairId= 0xB3E8;
	constexpr auto team = 0xF4;
	constexpr auto health = 0x100;
	constexpr auto vectorOrigin = 0x138;
	//for radar
	constexpr auto m_iTeamNum = 0xF4;
	constexpr auto m_bSpotted = 0x93D;
	constexpr auto m_bDormant = 0xED;
}

auto mem = Memory("csgo.exe");
const auto client = mem.GetModuleAddress("client.dll");

int main() {
	while (true) {
		std::thread radar_thread(radar_hack);
		std::thread trigger_handle_thread(trigger_handle);
		std::thread bunny_thread(bunny_handle);

		if (GetAsyncKeyState(VK_END))
			exit(0);

		radar_thread.join();
		trigger_handle_thread.join();
		bunny_thread.join();

		std::chrono::milliseconds timespan(5);
	}
	return 0;
}

void bunny_handle() {
	if (GetAsyncKeyState(VK_LCONTROL))
		bunny_hop();
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void bunny_hop() {
	const auto localPlayer = mem.Read<uintptr_t>(client + offsets::localPlayer);
	if (localPlayer) {
		const auto onGround = mem.Read<bool>(localPlayer + offsets::flags);
		if (onGround & (1 << 0))
			mem.Write<BYTE>(client + offsets::forceJump, 6);
	}
}

void trigger_handle() {
	if (GetAsyncKeyState(0x43))
		trigger_bot();
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void trigger_bot() {	
	const auto localPlayer = mem.Read<uintptr_t>(client + offsets::localPlayer);
	auto my_team = mem.Read<uintptr_t>(localPlayer + offsets::team);
	bool trigger_active = true;

	auto crosshair = mem.Read<uintptr_t>(localPlayer + offsets::crosshairId);
	auto entity = mem.Read<uintptr_t>(client + offsets::entityList + ((crosshair - 1) * 0x10));

	if (crosshair != 0 && crosshair < 64) {
		auto enemy_team = mem.Read<uintptr_t>(entity + offsets::team);
		auto enemy_health = mem.Read<uintptr_t>(entity + offsets::health);
		if (enemy_team != my_team && enemy_health > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(80));
			mem.Write<BYTE>(client + offsets::attack_lmb, 5);
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
			mem.Write<BYTE>(client + offsets::attack_lmb, 4);
		}
	}
}

void radar_hack() {
	const auto localPlayer = mem.Read<uintptr_t>(client + offsets::localPlayer);
	auto my_team = mem.Read<uintptr_t>(localPlayer + offsets::team);

	for (int i = 0; i < 33; i++) {
		auto entity = mem.Read<uintptr_t>(client + offsets::entityList + (i*0x10));
		auto enemy_team = mem.Read<uintptr_t>(entity+offsets::m_iTeamNum);
		if (my_team == enemy_team)
			continue;

		auto enemy_health = mem.Read<uintptr_t>(entity + offsets::health);
		if (enemy_health == 0)
			continue;

		bool is_enemy_spotted = mem.Read<bool>(entity + offsets::m_bSpotted);
		bool is_enemy_dormant = mem.Read<bool>(entity + offsets::m_bDormant);
		if (is_enemy_spotted || is_enemy_dormant)
			continue;

		mem.Write<BYTE>(entity + offsets::m_bSpotted, 1);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}