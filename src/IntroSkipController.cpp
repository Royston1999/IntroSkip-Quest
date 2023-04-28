#include "IntroSkipController.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "UnityEngine/Time.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/AudioSource.hpp"
#include "Config.hpp"
#include "main.hpp"

DEFINE_TYPE(IntroSkip, IntroSkipController);

using namespace UnityEngine;
using namespace GlobalNamespace;

namespace IntroSkip{
    void IntroSkipController::ctor(AudioTimeSyncController* audioTimeSyncController, IReadonlyBeatmapData* readonlyBeatmapData, IDifficultyBeatmap* difficultyBeatmap, PauseMenuManager* pauseMenuManager){
        INVOKE_CTOR();
        _audioTimeSyncController = audioTimeSyncController;
        _mapData = readonlyBeatmapData;
        _difficultyBeatmap = difficultyBeatmap;
        _leftController = pauseMenuManager->get_transform()->Find("MenuControllers/ControllerLeft")->GetComponent<GlobalNamespace::VRController*>();
        _rightController = pauseMenuManager->get_transform()->Find("MenuControllers/ControllerRight")->GetComponent<GlobalNamespace::VRController*>();
        _comboUIController = Resources::FindObjectsOfTypeAll<ComboUIController*>().LastOrDefault();
        getLogger().info("Constructed IntroSkip Controller");
    }

    void IntroSkipController::Initialize(){
        skipTimePairs = Utils::CalculateSkipTimePairs(_mapData, _difficultyBeatmap->get_level()->i_IPreviewBeatmapLevel()->get_songDuration());
        skipItr = skipTimePairs.begin();
        getLogger().info("Initialised IntroSkip Controller");
    }

    void IntroSkipController::Dispose(){
        skipTimePairs.clear();
        UnityEngine::Object::Destroy(_skipText);
        getLogger().info("Disposed IntroSkip Controller");
    }

    void IntroSkipController::Tick(){
        if (skipItr == skipTimePairs.end()) return;

        float currentTime = _audioTimeSyncController->songTime, bufferedCurrentTime = currentTime + 10 * UnityEngine::Time::get_deltaTime();
        bool triggersPressed = _leftController->get_triggerValue() > 0.85 && _rightController->get_triggerValue() > 0.85, notPaused = _audioTimeSyncController->state == 0;
        float skipStart = skipItr->first, skipEnd = skipItr->second;

        if (skipEnd <= bufferedCurrentTime) return iterateToNextPair(); // passed end of skippable range
        else if (skipStart >= currentTime) return; // not yet reached next skippable point
        else setSkipText(true); // woo skippable
        if (triggersPressed && notPaused && timeHeld >= requiredHoldTime) _audioTimeSyncController->audioSource->set_time(skipEnd); // skip to the end of the range
        if (triggersPressed) timeHeld += UnityEngine::Time::get_deltaTime(); // increase time held
        else if (timeHeld > 0) timeHeld = 0; // reset if no longer holding triggers
    }

    TMPro::TextMeshProUGUI* IntroSkipController::CreateSkipText(){
        auto text = QuestUI::BeatSaberUI::CreateText(_comboUIController->get_transform(), "Press Triggers to Skip", Vector2(0, 57));
        text->set_alignment(TMPro::TextAlignmentOptions::Center);
        text->get_transform()->set_localScale({6.0f, 6.0f, 0.0f});
        return text;
    }

    void IntroSkipController::setSkipText(bool value){
        if (_skipText == nullptr) _skipText = CreateSkipText();
        if (_skipText->get_gameObject()->get_active() != value) _skipText->get_gameObject()->SetActive(value);
    }

    void IntroSkipController::iterateToNextPair(){
        setSkipText(false);
        timeHeld = 0;
        skipItr++;
    }
}