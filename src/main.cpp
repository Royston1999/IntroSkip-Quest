#include "main.hpp"
#include <vector>

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"

#include "Config.hpp"

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
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/VRControllersInputManager.hpp"
#include "GlobalNamespace/ComboUIController.hpp"
#include "GlobalNamespace/VRController.hpp"

#include "HMUI/ViewController.hpp"

#include "UnityEngine/AudioSource.hpp"
#include "UnityEngine/XR/XRNode.hpp"
#include "UnityEngine/Resources.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "System/Threading/Tasks/Task_1.hpp"

using namespace GlobalNamespace;
using namespace UnityEngine::XR;
using namespace UnityEngine;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
DEFINE_CONFIG(IntroSkipConfig);
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

std::vector<std::pair<float, float>> criticalTimeValues;
std::vector<std::pair<float, float>>::iterator skipItr;
bool currentlySkippable, dataCollected;
System::Threading::Tasks::Task_1<IReadonlyBeatmapData*>* getMapData;
float songLength, lTriggerVal, rTriggerVal;
TMPro::TextMeshProUGUI* skipText = nullptr;

float LerpUnclamped(float a, float b, float t) {return a + (b - a) * t;}

std::vector<std::pair<float, float>> CalculateCritialTimeValues(IReadonlyBeatmapData* data){
    std::vector<float> mapValues;
    auto* notes = List<NoteData*>::New_ctor(); notes->AddRange(data->GetBeatmapDataItems<NoteData*>());
    auto* sliders = List<SliderData*>::New_ctor(); sliders->AddRange(data->GetBeatmapDataItems<SliderData*>());
    auto itr1 = notes->GetEnumerator();
    while (itr1.MoveNext()){
        auto* noteData = itr1.get_Current();
        if (noteData->get_scoringType() != -1 && noteData->get_scoringType() != 4) mapValues.push_back(noteData->get_time());
    }
    auto itr2 = sliders->GetEnumerator();
    while (itr2.MoveNext()){
        auto* sliderData = itr2.get_Current();
        if (sliderData->get_sliderType() == 1){
            if (sliderData->get_hasHeadNote()) mapValues.push_back(sliderData->get_time());
            for (int i = 1; i < sliderData->get_sliceCount(); i++){
                float t = i / (sliderData->get_sliceCount() - 1);
                mapValues.push_back(LerpUnclamped(sliderData->get_time(), sliderData->get_tailTime(), t));
            }
        }
    }
    std::sort(mapValues.begin(), mapValues.end(), [](auto &left, auto &right) { return left < right; });
    float currentTime = mapValues[0];
    std::vector<std::pair<float, float>> skippables;
    if (getIntroSkipConfig().skipIntro.GetValue()) if (currentTime > getIntroSkipConfig().minSkipTime.GetValue()) skippables.push_back(std::make_pair(0.1f, currentTime - 1.5f));
    if (getIntroSkipConfig().skipMiddle.GetValue()){
        for (auto& time : mapValues){
            if (time - currentTime > getIntroSkipConfig().minSkipTime.GetValue()) skippables.push_back(std::make_pair(currentTime, time - 1.5f));
            currentTime = time;
        }
    }
    if (getIntroSkipConfig().skipOutro.GetValue()) if(songLength - mapValues.back() > getIntroSkipConfig().minSkipTime.GetValue()) skippables.push_back(std::make_pair(mapValues.back(), songLength - 0.5f));
    mapValues.clear(); std::vector<float>().swap(mapValues); 
    return skippables;
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
    dataCollected = false;
    getMapData = self->GetTransformedBeatmapDataAsync();
}

MAKE_HOOK_MATCH(SongUpdate, &AudioTimeSyncController::Update, void, AudioTimeSyncController* self) {
    SongUpdate(self);
    if (getIntroSkipConfig().isEnabled.GetValue()){
        if (!dataCollected){
            if (!getMapData->get_IsCompleted()) return;
            criticalTimeValues = CalculateCritialTimeValues(getMapData->get_ResultOnSuccess());
            skipItr = criticalTimeValues.begin();
            dataCollected = true;
        }
        if (skipItr != criticalTimeValues.end()){
            float currentTime = self->dyn__songTime();
            if (skipItr->first < currentTime && !currentlySkippable){
                currentlySkippable = true;
                skipText = CreateSkipText();
                skipText->get_gameObject()->set_active(true);
            }
            else if (skipItr->second < currentTime && currentlySkippable){
                skipItr++;
                currentlySkippable = false;
                skipText->get_gameObject()->set_active(false);
            }
            else if (currentlySkippable && lTriggerVal > 0.85 && rTriggerVal > 0.85){
                self->dyn__audioSource()->set_time(skipItr->second);
                skipItr++;
                currentlySkippable = false;
                skipText->get_gameObject()->set_active(false);
            }
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

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if(firstActivation) 
    {
        UnityEngine::GameObject* container = QuestUI::BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
        UnityEngine::UI::Toggle* isEnabled = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().isEnabled);
        UnityEngine::UI::Toggle* skipIntro = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipIntro);
        UnityEngine::UI::Toggle* skipMiddle = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipMiddle);
        UnityEngine::UI::Toggle* skipOutro = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipOutro);
        QuestUI::IncrementSetting* minSkipTime = AddConfigValueIncrementFloat(container->get_transform(), getIntroSkipConfig().minSkipTime, 1, 0.1f, 2.5f, 10.0f);
        QuestUI::BeatSaberUI::AddHoverHint(isEnabled->get_gameObject(), "Enable the Intro Skip Mod");
        QuestUI::BeatSaberUI::AddHoverHint(skipIntro->get_gameObject(), "Enable the ability to skip the beginning of songs");
        QuestUI::BeatSaberUI::AddHoverHint(skipMiddle->get_gameObject(), "Enable the ability to skip the middle sections of songs");
        QuestUI::BeatSaberUI::AddHoverHint(skipOutro->get_gameObject(), "Enable the ability to skip the ending of songs");
        QuestUI::BeatSaberUI::AddHoverHint(minSkipTime->get_gameObject(), "Minimum amount of downtime required to be able to skip that section of the song");
    }
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    getIntroSkipConfig().Init(modInfo);
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), GameplayCoreSceneSetupData_ctor);
    INSTALL_HOOK(getLogger(), SongUpdate);
    INSTALL_HOOK(getLogger(), ControllerUpdate);
    getLogger().info("Installed all hooks!");
}