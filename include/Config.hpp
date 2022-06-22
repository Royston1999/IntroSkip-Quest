#pragma once
#include <string>

typedef struct config_t {
	std::string version = "1.1.0";
    bool isEnabled = true;
    bool skipIntro = true;
    bool skipMiddle = true;
    bool skipOutro = true;
    float minSkipTime = 5.0f;
    float minHoldTime = 0.5f;
	bool requiresBothTriggers = false;
	bool useOldUI = false;
} config_t;

extern config_t config;

bool LoadConfig();
void SaveConfig();