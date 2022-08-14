#pragma once

#include "SDK/SDK.hpp";
using namespace SDK;

namespace Globals
{
	bool bIsLateGame = true; // Set to true if late game amog us
	bool bisCreativeToolsEnabled = false; // beta
	bool bIsWarmupCountdownEnabled = true; // Set to true if you want countdown (waiting for players -> in 10 seconds aircraft, or just press F6)

	bool isEventPlaying = false; // this probably won't work in the future but set to true if you want marshmello event to play/playlist
	bool infAmmo = false; // set infammo (Reloading disabled for now)
	bool bPlaygroundEnabled = false;

	bool bWillSkipAircraft = false; // something
	float AircraftStartTime = 25; // amount of time for aircraft
	float WarmupCountdownEndTime = 900; // amount of time for warmup/waiting for players to end
}