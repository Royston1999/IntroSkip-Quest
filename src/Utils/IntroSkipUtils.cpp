#include "Utils/IntroSkipUtils.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/ObstacleData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "Config.hpp"
#include <type_traits>

using namespace GlobalNamespace;
using namespace System::Collections::Generic;
namespace IntroSkip::Utils {

    float LerpU(float a, float b, float t) {
        return a + (b - a) * t;
    }

    template<typename T>
    requires (std::is_convertible_v<T, BeatmapDataItem*>)
    ListW<T> GetBeatmapDataItems(IReadonlyBeatmapData* data) {
        return List_1<T>::New_ctor(data->GetBeatmapDataItems<T>(0));
    };

    SkipTimePairs CalculateSkipTimePairs(IReadonlyBeatmapData* beatmapData, float songLength) {
        SkipTimePairs skipTimePairs;
        std::vector<float> mapValues; mapValues.reserve(1500);
        for (auto& noteData : GetBeatmapDataItems<NoteData*>(beatmapData)){
            if (noteData->get_scoringType() != NoteData::ScoringType::Ignore) mapValues.push_back(noteData->get_time());
        }
        for (auto& sliderData : GetBeatmapDataItems<SliderData*>(beatmapData)){
            if (sliderData->get_sliderType() == SliderData::Type::Burst){
                int slices = sliderData->get_sliceCount();
                for (int i = 1; i < slices; i++) mapValues.push_back(LerpU(sliderData->get_time(), sliderData->get_tailTime(), (float)i / (slices - 1)));
            }
        }
        for (auto& obstacleData : GetBeatmapDataItems<ObstacleData*>(beatmapData)){
            float startIndex = obstacleData->get_lineIndex();
            float endIndex = startIndex + obstacleData->get_width() -1;
            if (startIndex <= 2 && endIndex >= 1){
                for (int i=0; i <= obstacleData->get_duration() / 2; i++) mapValues.push_back(obstacleData->get_time() + (i * 2));
                mapValues.push_back(obstacleData->get_time() + obstacleData->get_duration());
            }
        }
        if (!mapValues.empty()) {
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