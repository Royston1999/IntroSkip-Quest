#pragma once

#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "beatsaber-hook/shared/utils/typedefs-array.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"

using SkipTimePairs = std::vector<std::pair<float, float>>;

namespace IntroSkip::Utils{
    SkipTimePairs CalculateSkipTimePairs(GlobalNamespace::IReadonlyBeatmapData* beatmapData, float songLength);

    template<typename T>
    ArrayW<T> GetBeatmapDataItems(GlobalNamespace::IReadonlyBeatmapData* data){
        auto* beatmapDataItems = List<T>::New_ctor(data->GetBeatmapDataItems<T>(0)); 
        beatmapDataItems->items->max_length = beatmapDataItems->size;
        return beatmapDataItems->items;
    };

    float LerpU(float a, float b, float t);
}