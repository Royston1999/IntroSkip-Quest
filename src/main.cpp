#include "main.hpp"

#include "Config.hpp"

#include "HMUI/ViewController.hpp"

#include "lapiz/shared/zenject/Zenjector.hpp"
#include "Installers/IntroSkipInstaller.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Misc.hpp"

using namespace BSML;
using namespace BSML::Lite;
using namespace UnityEngine;

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

const Paper::ConstLoggerContext<sizeof(MOD_ID)>& getLogger() {
    static constexpr auto Logger = Paper::ConstLoggerContext(MOD_ID);
    return Logger;
}

INTRO_SKIP_EXPORT_FUNC void setup(CModInfo& info) {
    info = modInfo.to_c();

    getLogger().info("Completed setup!");
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if(!firstActivation) return;

    GameObject* container = CreateScrollableSettingsContainer(self->get_transform());
    ToggleSetting* isEnabled = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().isEnabled);
    ToggleSetting* skipIntro = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipIntro);
    ToggleSetting* skipMiddle = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipMiddle);
    ToggleSetting* skipOutro = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().skipOutro);
    SliderSetting* minSkipTime = AddConfigValueSliderIncrement(container->get_transform(), getIntroSkipConfig().minSkipTime, 0.1f, 2.5f, 10.0f);
    SliderSetting* minHoldTime = AddConfigValueSliderIncrement(container->get_transform(), getIntroSkipConfig().minHoldTime, 0.1f, 0.0f, 5.0f);
    auto format = [](float x) { return fmt::format("{:.1f} s", x); };
    minHoldTime->formatter = format;
    minSkipTime->formatter = format;
    ToggleSetting* bothTriggers = AddConfigValueToggle(container->get_transform(), getIntroSkipConfig().bothTriggers);

    AddHoverHint(isEnabled->get_gameObject(), "Enable the Intro Skip Mod");
    AddHoverHint(skipIntro->get_gameObject(), "Enable the ability to skip the beginning of songs");
    AddHoverHint(skipMiddle->get_gameObject(), "Enable the ability to skip the middle sections of songs");
    AddHoverHint(skipOutro->get_gameObject(), "Enable the ability to skip the ending of songs");
    AddHoverHint(minSkipTime->get_gameObject(), "Minimum amount of downtime required to be able to skip that section of the song");
    AddHoverHint(minHoldTime->get_gameObject(), "How long you you are required to press the triggers for before it skips");
    AddHoverHint(bothTriggers->get_gameObject(), "Require both triggers be pressed in order to skip. WHen disabled either trigger will skip");
}

// Called later on in the game loading - a good time to install function hooks
INTRO_SKIP_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    getIntroSkipConfig().Init(modInfo);
    BSML::Init();
    BSML::Register::RegisterMainMenuViewControllerMethod("Intro Skip", "Intro Skip", "skip those intros or whatever", &DidActivate);
    auto zenjector = Lapiz::Zenject::Zenjector::Get();
    zenjector->Expose<GlobalNamespace::ComboUIController*>("Environment");
    zenjector->Install<IntroSkip::Installers::IntroSkipInstaller*>(Lapiz::Zenject::Location::StandardPlayer);
}