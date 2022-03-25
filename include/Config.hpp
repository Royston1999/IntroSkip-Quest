#pragma once

#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(IntroSkipConfig,
    CONFIG_VALUE(version, std::string, "Config Verision", "1.0.0");
    CONFIG_VALUE(isEnabled, bool, "Enable Mod", true);
    CONFIG_VALUE(skipIntro, bool, "Enable Intro Skipping", true);
    CONFIG_VALUE(skipMiddle, bool, "Enable Middle Skipping", true);
    CONFIG_VALUE(skipOutro, bool, "Enable Outro Skipping", true);
    CONFIG_VALUE(minSkipTime, float, "Minimum Skip Time", 5.0f);

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(version);
        CONFIG_INIT_VALUE(isEnabled);
        CONFIG_INIT_VALUE(skipIntro);
        CONFIG_INIT_VALUE(skipMiddle);
        CONFIG_INIT_VALUE(skipOutro);
        CONFIG_INIT_VALUE(minSkipTime);
    )
)