#include "main.hpp"

#include "Config.hpp"

#include "HMUI/ViewController.hpp"

#include "lapiz/shared/zenject/Zenjector.hpp"
#include "Installers/IntroSkipInstaller.hpp"

#include "questui/shared/QuestUI.hpp"

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
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    getIntroSkipConfig().Init(modInfo);
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    auto zenjector = Lapiz::Zenject::Zenjector::Get();
    zenjector->Install<IntroSkip::Installers::IntroSkipInstaller*, GlobalNamespace::StandardGameplayInstaller*>();
}