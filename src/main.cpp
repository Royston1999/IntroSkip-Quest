#include "main.hpp"
#include <vector>

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"

#include "Config.hpp"

#ifndef USE_CODEGEN_FIELDS
#define USE_CODEGEN_FIELDS
#endif

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

using namespace GlobalNamespace;
using namespace UnityEngine::XR;
using namespace UnityEngine;

DEFINE_CONFIG(IntroSkipConfig);

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

std::vector<std::pair<float, float>> skipTimePairs;
std::vector<std::pair<float, float>>::iterator skipItr;
bool currentlySkippable, isMulti;
float songLength, lTriggerVal, rTriggerVal;
TMPro::TextMeshProUGUI* skipText = nullptr;
float timeHeld = 0;

float LerpU(float a, float b, float t) {return a + (b - a) * t;}

template<class T>
List<T>* GetBeatmapDataItems(IReadonlyBeatmapData* data){
    auto* beatmapDataItems = List<T>::New_ctor(); 
    beatmapDataItems->AddRange(data->GetBeatmapDataItems<T>());
    return beatmapDataItems;
}

template<class T>
void ClearVector(std::vector<T>* vector){
    vector->clear(); std::vector<T>().swap(*vector);
}

void CalculateSkipTimePairs(IReadonlyBeatmapData* data){
    std::vector<float> mapValues; mapValues.reserve(1500);
    List<NoteData*>* noteDataItems = GetBeatmapDataItems<NoteData*>(data);
    for (int i = 0; i < noteDataItems->size; i++){
        NoteData* noteData = noteDataItems->items[i];
        if (noteData->scoringType != -1 && noteData->scoringType != 4) mapValues.push_back(noteData->time);
    }
    List<SliderData*>* sliderDataItems = GetBeatmapDataItems<SliderData*>(data);
    for (int i = 0; i < sliderDataItems->size; i++){
        SliderData* sliderData = sliderDataItems->items[i];
        if (sliderData->sliderType == 1){
            int slices = sliderData->sliceCount;
            if (sliderData->hasHeadNote) mapValues.push_back(sliderData->time);
            for (int i = 1; i < slices; i++) mapValues.push_back(LerpU(sliderData->time, sliderData->tailTime, i / (slices - 1)));
        }
    }
    List<ObstacleData*>* obstacleDataItems = GetBeatmapDataItems<ObstacleData*>(data);
    for (int i = 0; i < obstacleDataItems->size; i++){
        ObstacleData* obstacleData = obstacleDataItems->items[i];
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
        ClearVector<float>(&mapValues);
    }
}

TMPro::TextMeshProUGUI* CreateSkipText(){
    auto transform = QuestUI::ArrayUtil::Last(Resources::FindObjectsOfTypeAll<ComboUIController*>())->get_transform();
    auto text = QuestUI::BeatSaberUI::CreateText(transform, "Press Triggers to Skip", Vector2(0, 57));
    text->set_alignment(TMPro::TextAlignmentOptions::Center);
    text->get_transform()->set_localScale({6.0f, 6.0f, 0.0f});
    return text;
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(GameplayCoreSceneSetupData_ctor, "", "GameplayCoreSceneSetupData", ".ctor", void, GameplayCoreSceneSetupData* self, IDifficultyBeatmap* difficultyBeatmap, IPreviewBeatmapLevel* previewBeatmapLevel, GameplayModifiers* gameplayModifiers, PlayerSpecificSettings* playerSpecificSettings, PracticeSettings* practiceSettings, bool useTestNoteCutSoundEffects, EnvironmentInfoSO* environmentInfo, ColorScheme* colorScheme, MainSettingsModelSO* mainSettingsModel)
{
    GameplayCoreSceneSetupData_ctor(self, difficultyBeatmap, previewBeatmapLevel, gameplayModifiers, playerSpecificSettings, practiceSettings, useTestNoteCutSoundEffects, environmentInfo, colorScheme, mainSettingsModel);
    songLength = previewBeatmapLevel->get_songDuration();
    skipText = nullptr; currentlySkippable = false; isMulti = false;
}

MAKE_HOOK_FIND_CLASS_UNSAFE_INSTANCE(BeatmapData_Init, "", "BeatmapCallbacksController", ".ctor", void, BeatmapCallbacksController* self, BeatmapCallbacksController::InitData* initData)
{
    BeatmapData_Init(self, initData);
    ClearVector<std::pair<float, float>>(&skipTimePairs);
    CalculateSkipTimePairs(initData->dyn_beatmapData());
    skipItr = skipTimePairs.begin();
}

MAKE_HOOK_MATCH(SongUpdate, &AudioTimeSyncController::Update, void, AudioTimeSyncController* self) {
    SongUpdate(self);
    if (getIntroSkipConfig().isEnabled.GetValue() && !isMulti){
        if (skipItr != skipTimePairs.end()){
            float timeToHold = getIntroSkipConfig().minHoldTime.GetValue();
            float currentTime = self->dyn__songTime();
            if (skipItr->first < currentTime && !currentlySkippable){
                currentlySkippable = true;
                if (skipText == nullptr) skipText = CreateSkipText();
                skipText->get_gameObject()->set_active(true);
            }
            else if (skipItr->second < currentTime && currentlySkippable){
                skipItr++;
                currentlySkippable = false;
                skipText->get_gameObject()->set_active(false);
            }
            else if (currentlySkippable && lTriggerVal > 0.85 && rTriggerVal > 0.85 && self->dyn__state() == 0 && timeHeld >= timeToHold){
                self->dyn__audioSource()->set_time(skipItr->second);
                skipItr++;
                currentlySkippable = false;
                skipText->get_gameObject()->set_active(false);
            }
            if (lTriggerVal > 0.85 && rTriggerVal > 0.85) timeHeld += UnityEngine::Time::get_deltaTime();
            else if (timeHeld > 0) timeHeld = 0;
        }
    }
}

MAKE_HOOK_MATCH(ControllerUpdate, &VRController::Update, void, VRController* self) {
    float trigger = self->get_triggerValue();
    auto node = self->dyn__node();
    if (node == XRNode::LeftHand) lTriggerVal = trigger;
    if (node == XRNode::RightHand) rTriggerVal = trigger;
    ControllerUpdate(self);
}

MAKE_HOOK_MATCH(isMultiplayer, &MultiplayerLocalActiveClient::Start, void, MultiplayerLocalActiveClient* self){
    isMultiplayer(self);
    isMulti = true;
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if(firstActivation) 
    {
        UnityEngine::GameObject* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
        UnityEngine::UI::Toggle* isEnabled = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().isEnabled);
        UnityEngine::UI::Toggle* skipIntro = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipIntro);
        UnityEngine::UI::Toggle* skipMiddle = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipMiddle);
        UnityEngine::UI::Toggle* skipOutro = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipOutro);
        QuestUI::IncrementSetting* minSkipTime = AddConfigValueIncrementFloat(container->get_transform(), getIntroSkipConfig().minSkipTime, 1, 0.1f, 2.5f, 10.0f);
        QuestUI::IncrementSetting* minHoldTime = AddConfigValueIncrementFloat(container->get_transform(), getIntroSkipConfig().minHoldTime, 1, 0.1f, 0.0f, 5.0f);
        QuestUI::BeatSaberUI::AddHoverHint(isEnabled->get_gameObject(), "Enable the Intro Skip Mod");
        QuestUI::BeatSaberUI::AddHoverHint(skipIntro->get_gameObject(), "Enable the ability to skip the beginning of songs");
        QuestUI::BeatSaberUI::AddHoverHint(skipMiddle->get_gameObject(), "Enable the ability to skip the middle sections of songs");
        QuestUI::BeatSaberUI::AddHoverHint(skipOutro->get_gameObject(), "Enable the ability to skip the ending of songs");
        QuestUI::BeatSaberUI::AddHoverHint(minSkipTime->get_gameObject(), "Minimum amount of downtime required to be able to skip that section of the song");
        QuestUI::BeatSaberUI::AddHoverHint(minHoldTime->get_gameObject(), "How long you you are required to press the triggers for before it skips");
    }
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    Modloader::requireMod("SmoothedController");
    il2cpp_functions::Init();
    getIntroSkipConfig().Init(modInfo);
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), GameplayCoreSceneSetupData_ctor);
    INSTALL_HOOK(getLogger(), SongUpdate);
    INSTALL_HOOK(getLogger(), ControllerUpdate);
    INSTALL_HOOK(getLogger(), BeatmapData_Init);
    INSTALL_HOOK(getLogger(), isMultiplayer);
    getLogger().info("Installed all hooks!");
}