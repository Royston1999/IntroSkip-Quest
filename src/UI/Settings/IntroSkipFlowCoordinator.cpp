#include "UI/Settings/IntroSkipFlowCoordinator.hpp"
#include "UI/Settings/IntroSkipSettingsViewController.hpp"

#include "questui/shared/BeatSaberUI.hpp"
//#include "Utils/UIUtils.hpp"

#include "HMUI/TitleViewController.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"

DEFINE_TYPE(IntroSkip::UI, IntroSkipFlowCoordinator);

using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace QuestUI;
using namespace QuestUI::BeatSaberUI;

namespace IntroSkip::UI
{
	void IntroSkipFlowCoordinator::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
	{
		if (firstActivation)
		{
			introSkipSettingsViewController = reinterpret_cast<HMUI::ViewController*>(CreateViewController<IntroSkipSettingsViewController*>());
			
			SetTitle(il2cpp_utils::newcsstr("Intro Skip Settings"), ViewController::AnimationType::Out);
			showBackButton = true;

			ProvideInitialViewControllers(introSkipSettingsViewController, nullptr, nullptr, nullptr, nullptr);
		}
	}

	void IntroSkipFlowCoordinator::BackButtonWasPressed(HMUI::ViewController* topViewController)
	{
		parentFlowCoordinator->DismissFlowCoordinator(this, ViewController::AnimationDirection::Horizontal, nullptr, false);
	}
}