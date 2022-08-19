#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "hooks.hpp"
#include "config.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SliderData.hpp"
#include "GlobalNamespace/ObstacleData.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/ComboUIController.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/BeatmapCallbacksController.hpp"
#include "GlobalNamespace/BeatmapCallbacksController_InitData.hpp"
#include "GlobalNamespace/MultiplayerLocalActiveClient.hpp"

#include "HMUI/ViewController.hpp"

#include "UnityEngine/AudioSource.hpp"
#include "UnityEngine/XR/XRNode.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Time.hpp"

#include "System/Action_1.hpp"

#include "TMPro/TextMeshProUGUI.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"

//to do
//separate this shit into more files


std::vector<std::pair<float, float>> skipTimePairs;
std::vector<std::pair<float, float>>::iterator skipItr;
bool currentlySkippable, isMulti;
float songLength, lTriggerVal, rTriggerVal;
float timeHeld = 0;

TMPro::TextMeshProUGUI* skipText = nullptr;

float LerpU(float a, float b, float t) {return a + (b - a) * t;}

template<class T>
ArrayW<T> GetBeatmapDataItems(GlobalNamespace::IReadonlyBeatmapData* data){
    auto* beatmapDataItems = List<T>::New_ctor(); 
    beatmapDataItems->AddRange(data->GetBeatmapDataItems<T>());
    beatmapDataItems->items->max_length = beatmapDataItems->size;
    return beatmapDataItems->items;
}

template<class T>
void ClearVector(std::vector<T>* vector){
    vector->clear(); std::vector<T>().swap(*vector);
}

void CalculateSkipTimePairs(GlobalNamespace::IReadonlyBeatmapData* beatmapData){
    std::vector<float> mapValues; mapValues.reserve(1500);
    for (auto& noteData : GetBeatmapDataItems<GlobalNamespace::NoteData*>(beatmapData)){
        if (noteData->scoringType != -1) mapValues.push_back(noteData->time);
    }
    for (auto& sliderData : GetBeatmapDataItems<GlobalNamespace::SliderData*>(beatmapData)){
        if (sliderData->sliderType == 1){
            int slices = sliderData->sliceCount;
            for (int i = 1; i < slices; i++) mapValues.push_back(LerpU(sliderData->time, sliderData->tailTime, i / (slices - 1)));
        }
    }
    for (auto& obstacleData : GetBeatmapDataItems<GlobalNamespace::ObstacleData*>(beatmapData)){
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
        float skipIntro = config.skipIntro, skipMiddle = config.skipMiddle;
        float skipOutro = config.skipOutro, minSkipTime = config.minSkipTime;
        if (skipIntro && currentTime > minSkipTime) skipTimePairs.push_back(std::make_pair(0.1f, currentTime - 1.5f));
        if (skipMiddle){
            for (auto& time : mapValues){
                if (time - currentTime > minSkipTime) skipTimePairs.push_back(std::make_pair(currentTime, time - 1.5f));
                currentTime = time;
            }
        }
        if (skipOutro && songLength - mapValues.back() > minSkipTime) skipTimePairs.push_back(std::make_pair(mapValues.back(), songLength - 0.5f));
        ClearVector<float>(&mapValues);
    }
}

TMPro::TextMeshProUGUI* CreateSkipText(){

    std::string textToShow = "Press Trigger To Skip";
    if(config.requiresBothTriggers) textToShow = "Press Triggers To Skip";

    auto transform = QuestUI::ArrayUtil::Last(UnityEngine::Resources::FindObjectsOfTypeAll<GlobalNamespace::ComboUIController*>())->get_transform();
    if(config.useOldUI){
        auto text = QuestUI::BeatSaberUI::CreateText(transform, textToShow, UnityEngine::Vector2(0, 57));
        text->set_alignment(TMPro::TextAlignmentOptions::Center);
        text->get_transform()->set_localScale({6.0f, 6.0f, 0.0f});
        text->set_color({1,1,1,0});
        return text;
    }
    auto text = QuestUI::BeatSaberUI::CreateText(transform, textToShow, false, UnityEngine::Vector2(89, 61.5));
    text->set_alignment(TMPro::TextAlignmentOptions::Center);
    text->set_fontSize(15.0f);
    text->get_transform()->set_localScale({2.8f, 2.8f, 0.0f});
    text->set_color({1,1,1,0});
    return text;
}

custom_types::Helpers::Coroutine FadeTextToFullAlpha(TMPro::TextMeshProUGUI* text, float t, bool changeActiveState)
{
    if(!text)
        co_return;
    UnityEngine::Color colour = text->get_color();
    while(text->get_color().a < 1.0){
        if(!text)
            break;
        text->set_color({colour.r,colour.g,colour.b, text->get_color().a + (UnityEngine::Time::get_deltaTime() / t)});
        co_yield nullptr;
    }
    if(changeActiveState && text)
        skipText->get_gameObject()->set_active(true);
    co_return;
}

custom_types::Helpers::Coroutine FadeTextToZeroAlpha(TMPro::TextMeshProUGUI* text, float t, bool changeActiveState)
{
    if(!text)
        co_return;
    UnityEngine::Color colour = text->get_color();
    while(text->get_color().a > 0.0){
        if(!text)
            break;
        text->set_color({colour.r,colour.g,colour.b, text->get_color().a - (UnityEngine::Time::get_deltaTime() / t)});
        co_yield nullptr;
    }
    if(changeActiveState && text)
        skipText->get_gameObject()->set_active(false);
    co_return;
}

void FadeTextIn(){
    if (skipText == nullptr) skipText = CreateSkipText();
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FadeTextToFullAlpha(skipText, 0.4, true)));
}

void FadeTextOut(){
    if (skipText == nullptr) return;
    GlobalNamespace::SharedCoroutineStarter::get_instance()->StartCoroutine(custom_types::Helpers::CoroutineHelper::New(FadeTextToZeroAlpha(skipText, 0.4, true)));
    
}

bool CorrectControllersHeld(){
    if(config.requiresBothTriggers)
        return (lTriggerVal > 0.85 && rTriggerVal > 0.85);
    return (lTriggerVal > 0.85 || rTriggerVal > 0.85);
}

MAKE_AUTO_HOOK_MATCH(SongUpdate, &GlobalNamespace::AudioTimeSyncController::Update, void, GlobalNamespace::AudioTimeSyncController* self) {
    SongUpdate(self);
    if (config.isEnabled && !isMulti){
        if (skipItr != skipTimePairs.end()){
            float timeToHold = config.minHoldTime;
            float currentTime = self->dyn__songTime();

            if (skipItr->first < currentTime && !currentlySkippable){
                currentlySkippable = true;
                FadeTextIn();
            }
            else if (skipItr->second < currentTime && currentlySkippable){
                skipItr++;
                currentlySkippable = false;
                FadeTextOut();
            }
            else if (currentlySkippable && CorrectControllersHeld() && self->dyn__state() == 0 && timeHeld >= timeToHold){
                self->dyn__audioSource()->set_time(skipItr->second);
                skipItr++;
                currentlySkippable = false;
                FadeTextOut();
            }
            if (CorrectControllersHeld()) timeHeld += UnityEngine::Time::get_deltaTime();
            else if (timeHeld > 0) timeHeld = 0;
        }
    }
}

MAKE_AUTO_HOOK_MATCH(ControllerUpdate, &GlobalNamespace::VRController::Update, void, GlobalNamespace::VRController* self) {
    float trigger = self->get_triggerValue();
    auto node = self->dyn__node();
    if (node == UnityEngine::XR::XRNode::LeftHand) lTriggerVal = trigger;
    if (node == UnityEngine::XR::XRNode::RightHand) rTriggerVal = trigger;
    ControllerUpdate(self);
}

MAKE_AUTO_HOOK_MATCH(isMultiplayer, &GlobalNamespace::MultiplayerLocalActiveClient::Start, void, GlobalNamespace::MultiplayerLocalActiveClient* self){
    isMultiplayer(self);
    isMulti = true;
}

MAKE_AUTO_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GlobalNamespace::GameplayCoreSceneSetupData* self, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap, GlobalNamespace::IPreviewBeatmapLevel* previewBeatmapLevel, GlobalNamespace::GameplayModifiers* gameplayModifiers, GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, GlobalNamespace::PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, GlobalNamespace::EnvironmentInfoSO* environmentInfo, GlobalNamespace::ColorScheme* colorScheme, GlobalNamespace::MainSettingsModelSO* mainSettingsModel)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme, mainSettingsModel);
    songLength = previewBeatmapLevel->get_songDuration();
    skipText = nullptr; currentlySkippable = false; isMulti = false;
}

MAKE_AUTO_HOOK_FIND_CLASS_UNSAFE_INSTANCE(BeatmapData_Init, "", "BeatmapCallbacksController", ".ctor", void, GlobalNamespace::BeatmapCallbacksController* self, GlobalNamespace::BeatmapCallbacksController::InitData* initData)
{
    BeatmapData_Init(self, initData);
    ClearVector<std::pair<float, float>>(&skipTimePairs);
    CalculateSkipTimePairs(initData->dyn_beatmapData());
    skipItr = skipTimePairs.begin();
}
