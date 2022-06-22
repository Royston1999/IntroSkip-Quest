#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "custom-types/shared/register.hpp"
#include "questui/shared/QuestUI.hpp"

#include "UI/Settings/IntroSkipFlowCoordinator.hpp"

#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/ImageView.hpp"

#include "hooks.hpp"
#include "logging.hpp"

#include "config.hpp"


ModInfo modInfo;


extern "C" void setup(ModInfo& info)
{
	info.id = ID;
	info.version = VERSION;

	modInfo = info;
}

extern "C" void load()
{
	il2cpp_functions::Class_Init(classof(HMUI::ImageView*));
    il2cpp_functions::Class_Init(classof(HMUI::CurvedTextMeshPro*));
	Logger& logger = IntroSkip::Logging::getLogger();
	logger.info("Loading IntroSkip!");
	QuestUI::Init();
	
	// if config load fails, save the config so it will work next time
	if (!LoadConfig())
		SaveConfig();
	Hooks::InstallHooks(logger);

	custom_types::Register::AutoRegister();

	QuestUI::Register::RegisterModSettingsFlowCoordinator<IntroSkip::UI::IntroSkipFlowCoordinator*>({"Intro Skip", VERSION});
}