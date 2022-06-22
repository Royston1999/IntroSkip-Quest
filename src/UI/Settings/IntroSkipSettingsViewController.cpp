#include "UI/Settings/IntroSkipSettingsViewController.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "config.hpp"
#include "logging.hpp"


DEFINE_TYPE(IntroSkip::UI, IntroSkipSettingsViewController);

using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace QuestUI;
using namespace QuestUI::BeatSaberUI;

// simple toggle macro because typing the same thing every time is dumb
#define TOGGLE(name, displayName) \
	CreateToggle(container->get_transform(), displayName, config.name, [](bool value) -> void { \
		config.name = value; \
		SaveConfig(); \
	});

#define Increment(name, displayName, decimals, increment, min, max) \
	CreateIncrementSetting(container->get_transform(), displayName, decimals, increment, config.name, min, max, \
        [](float value) { \
            config.name = value; \
            SaveConfig(); \
        } \
    );

namespace IntroSkip::UI
{
	void IntroSkipSettingsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
	{
		if (firstActivation)
		{
			GameObject* container = CreateScrollableSettingsContainer(get_transform());

            std::vector<StringW> triggerTypeStringWVector = { 
                "One Trigger",
                "Both Triggers",
                "Disable",
            };
            int defaultVal = (int)config.requiresBothTriggers;
            if(!config.isEnabled) defaultVal = 2;
            StringW triggerTypeStringW = triggerTypeStringWVector[defaultVal];
            QuestUI::BeatSaberUI::CreateDropdown(container->get_transform(), "Trigger Options", triggerTypeStringW, triggerTypeStringWVector,
                [triggerTypeStringWVector](std::string value) {
                    if (value == triggerTypeStringWVector[0]) {
                        config.isEnabled = true;
                        config.requiresBothTriggers = false;
                    }else if (value == triggerTypeStringWVector[1]) {
                        config.isEnabled = true;
                        config.requiresBothTriggers = true;
                    }else {
                        config.isEnabled = false;
					}
                    SaveConfig();
                }
            );
			Toggle* skipIntroObj = TOGGLE(skipIntro, "Enable Intro Skipping");
			Toggle* skipMiddleObj = TOGGLE(skipMiddle, "Enable Middle Skipping");
			Toggle* skipOutroObj = TOGGLE(skipOutro, "Enable Outro Skipping");
			Toggle* useOldUIObj = TOGGLE(useOldUI, "Use Old UI");
			IncrementSetting* minSkipTimeObj = Increment(minSkipTime, "Minimum Skip Time", 1, 0.1f, 2.5f, 10.0f);
			IncrementSetting* minHoldTimeObj = Increment(minSkipTime, "Minimum Hold Time", 1, 0.1f, 0.0f, 5.0f);

            QuestUI::BeatSaberUI::AddHoverHint(skipIntroObj->get_gameObject(), "Enable the ability to skip the beginning of songs");
            QuestUI::BeatSaberUI::AddHoverHint(skipMiddleObj->get_gameObject(), "Enable the ability to skip the middle sections of songs");
            QuestUI::BeatSaberUI::AddHoverHint(skipOutroObj->get_gameObject(), "Enable the ability to skip the ending of songs");
            QuestUI::BeatSaberUI::AddHoverHint(useOldUIObj->get_gameObject(), "Use the old UI layout");
            QuestUI::BeatSaberUI::AddHoverHint(minSkipTimeObj->get_gameObject(), "Minimum amount of downtime required to be able to skip that section of the song");
            QuestUI::BeatSaberUI::AddHoverHint(minHoldTimeObj->get_gameObject(), "How long you you are required to press the triggers for before it skips");

		}
	}
}