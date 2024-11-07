#include "IntroSkipController.hpp"
#include "UnityEngine/Time.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/AudioSource.hpp"
#include "Config.hpp"
#include "main.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "UnityEngine/UI/CanvasScaler.hpp"
#include "VRUIControls/VRGraphicRaycaster.hpp"
#include "HMUI/CurvedCanvasSettings.hpp"

DEFINE_TYPE(IntroSkip, IntroSkipController);

using namespace UnityEngine;
using namespace GlobalNamespace;

namespace IntroSkip{
    void IntroSkipController::ctor(AudioTimeSyncController* audioTimeSyncController, IReadonlyBeatmapData* readonlyBeatmapData, PauseMenuManager* pauseMenuManager, ComboUIController* comboUIController) {
        INVOKE_CTOR();
        _audioTimeSyncController = audioTimeSyncController;
        _mapData = readonlyBeatmapData;
        _leftController = pauseMenuManager->get_transform()->Find("MenuControllers/ControllerLeft")->GetComponent<GlobalNamespace::VRController*>();
        _rightController = pauseMenuManager->get_transform()->Find("MenuControllers/ControllerRight")->GetComponent<GlobalNamespace::VRController*>();
        _comboUIController = comboUIController;
        getLogger().info("Constructed IntroSkip Controller");
    }

    void IntroSkipController::Initialize() {
        skipTimePairs = Utils::CalculateSkipTimePairs(_mapData, _audioTimeSyncController->get_songLength());
        skipItr = skipTimePairs.begin();
        requiredHoldTime = getIntroSkipConfig().minHoldTime.GetValue();
        requiresBothTriggers = getIntroSkipConfig().bothTriggers.GetValue();
        _skipText = CreateSkipText();
        getLogger().info("Initialised IntroSkip Controller");
    }

    void IntroSkipController::Dispose() {
        skipTimePairs.clear();
        UnityEngine::Object::Destroy(_skipText);
        getLogger().info("Disposed IntroSkip Controller");
    }

    void IntroSkipController::Tick() {
        if (skipItr == skipTimePairs.end()) return;

        float currentTime = _audioTimeSyncController->get_songTime(), bufferedCurrentTime = currentTime + 10 * UnityEngine::Time::get_deltaTime();
        bool triggersPressed = this->triggersPressed(), notPaused = _audioTimeSyncController->get_state() == AudioTimeSyncController::State::Playing;
        float skipStart = skipItr->first, skipEnd = skipItr->second;

        if (skipEnd <= bufferedCurrentTime) return iterateToNextPair(); // passed end of skippable range
        else if (skipStart >= currentTime) return; // not yet reached next skippable point
        else setSkipText(true); // woo skippable
        if (triggersPressed && notPaused && timeHeld >= requiredHoldTime) _audioTimeSyncController->_audioSource->set_time(skipEnd); // skip to the end of the range
        if (triggersPressed) timeHeld += UnityEngine::Time::get_deltaTime(); // increase time held
        else if (timeHeld > 0) timeHeld = 0; // reset if no longer holding triggers
    }

    TMPro::TextMeshProUGUI* IntroSkipController::CreateSkipText() {
        auto text = BSML::Helpers::CreateText<TMPro::TextMeshProUGUI*>(_comboUIController->get_transform()->get_parent(), "Press Triggers to Skip", {0, 0});
        _comboUIController->get_transform()->get_parent()->get_gameObject()->AddComponent<Canvas*>(); // 6 hours of my life wasted on this line
        text->get_transform()->set_localPosition({0.0f, 1.95f, 0.0f});
        text->set_alignment(TMPro::TextAlignmentOptions::Center);
        text->get_transform()->set_localScale({0.025f, 0.025f, 0.025f});
        text->set_fontSize(8);
        text->get_gameObject()->set_active(false);
        return text;
    }

    void IntroSkipController::setSkipText(bool value) {
        if (_skipText && _skipText->get_gameObject()->get_activeSelf() != value) _skipText->get_gameObject()->set_active(value);
    }

    void IntroSkipController::iterateToNextPair() {
        setSkipText(false);
        timeHeld = 0;
        skipItr++;
    }

    bool IntroSkipController::triggersPressed() {
        bool leftPressed = _leftController->triggerValue > 0.85f;
        bool rightPressed = _rightController->triggerValue > 0.85f;

        return requiresBothTriggers ? leftPressed && rightPressed : leftPressed || rightPressed;
    }
}