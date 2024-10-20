#pragma once

#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"

using SkipTimePairs = std::vector<std::pair<float, float>>;

namespace IntroSkip::Utils{
    SkipTimePairs CalculateSkipTimePairs(GlobalNamespace::IReadonlyBeatmapData* beatmapData, float songLength);
}