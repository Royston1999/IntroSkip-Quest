#include "Utils/IntroSkipUtils.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/ObstacleData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "Config.hpp"

using namespace GlobalNamespace;

namespace IntroSkip::Utils{

    float LerpU(float a, float b, float t) {
        return a + (b - a) * t;
    }

    SkipTimePairs CalculateSkipTimePairs(IReadonlyBeatmapData* beatmapData, float songLength){
        SkipTimePairs skipTimePairs;
        std::vector<float> mapValues; mapValues.reserve(1500);
        for (auto& noteData : GetBeatmapDataItems<NoteData*>(beatmapData)){
            if (noteData->scoringType != -1) mapValues.push_back(noteData->time);
        }
        for (auto& sliderData : GetBeatmapDataItems<SliderData*>(beatmapData)){
            if (sliderData->sliderType == 1){
                int slices = sliderData->sliceCount;
                for (int i = 1; i < slices; i++) mapValues.push_back(LerpU(sliderData->time, sliderData->tailTime, i / (slices - 1)));
            }
        }
        for (auto& obstacleData : GetBeatmapDataItems<ObstacleData*>(beatmapData)){
            float startIndex = obstacleData->lineIndex;
            float endIndex = startIndex + obstacleData->width -1;
            if (startIndex <= 2 && endIndex >= 1){
                for (int i=0; i <= obstacleData->duration / 2; i++) mapValues.push_back(obstacleData->time + (i * 2));
                mapValues.push_back(obstacleData->time + obstacleData->duration);
            }
        }
        if (mapValues.empty()) skipTimePairs.push_back(std::make_pair(0.1f, songLength - 0.5f));
        else {
            std::sort(mapValues.begin(), mapValues.end(), [](auto &left, auto &right) { return left < right; });
            float currentTime = mapValues[0];
            float skipIntro = getIntroSkipConfig().skipIntro.GetValue(), skipMiddle = getIntroSkipConfig().skipMiddle.GetValue();
            float skipOutro = getIntroSkipConfig().skipOutro.GetValue(), minSkipTime = getIntroSkipConfig().minSkipTime.GetValue();
            if (skipIntro && currentTime > minSkipTime) skipTimePairs.push_back(std::make_pair(0.1f, currentTime - 1.5f));
            if (skipMiddle){
                for (auto& time : mapValues){
                    if (time - currentTime > minSkipTime) skipTimePairs.push_back(std::make_pair(currentTime, time - 1.5f));
                    currentTime = time;
                }
            }
            if (skipOutro && songLength - mapValues.back() > minSkipTime) skipTimePairs.push_back(std::make_pair(mapValues.back(), songLength - 0.5f));
        }
        return skipTimePairs;
    }
}