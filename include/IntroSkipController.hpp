#pragma once

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "System/IDisposable.hpp"
#include "Zenject/IInitializable.hpp"
#include "custom-types/shared/macros.hpp"
#include "lapiz/shared/macros.hpp"
#include "GlobalNamespace/ComboUIController.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include <vector>
#include "Utils/IntroSkipUtils.hpp"
#include "Zenject/ITickable.hpp"

#define INTERFACES                                                        \
    {                                                                     \
        classof(System::IDisposable*), classof(Zenject::IInitializable*), classof(Zenject::ITickable*)\
    }

DECLARE_CLASS_CODEGEN_INTERFACES(IntroSkip, IntroSkipController, Il2CppObject, std::vector<Il2CppClass*>(INTERFACES),
    DECLARE_INSTANCE_FIELD(GlobalNamespace::AudioTimeSyncController*, _audioTimeSyncController);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, _skipText);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::IReadonlyBeatmapData*, _mapData);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::VRController*, _leftController);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::VRController*, _rightController);
    DECLARE_INSTANCE_FIELD(GlobalNamespace::ComboUIController*, _comboUIController);
    DECLARE_CTOR(ctor, 
                GlobalNamespace::AudioTimeSyncController* audioTimeSyncController, 
                GlobalNamespace::IReadonlyBeatmapData* readonlyBeatmapData, 
                GlobalNamespace::PauseMenuManager* pauseMenuManager,
                GlobalNamespace::ComboUIController* comboUIController
                );
    DECLARE_OVERRIDE_METHOD(void, Initialize, il2cpp_utils::il2cpp_type_check::MetadataGetter<&::Zenject::IInitializable::Initialize>::methodInfo());
    DECLARE_OVERRIDE_METHOD(void, Dispose, il2cpp_utils::il2cpp_type_check::MetadataGetter<&::System::IDisposable::Dispose>::methodInfo());
    DECLARE_OVERRIDE_METHOD(void, Tick, il2cpp_utils::il2cpp_type_check::MetadataGetter<&::Zenject::ITickable::Tick>::methodInfo());
    private:
        SkipTimePairs skipTimePairs;
        SkipTimePairs::iterator skipItr;
        float requiredHoldTime, timeHeld;
        TMPro::TextMeshProUGUI* CreateSkipText();
        void setSkipText(bool value);
        void iterateToNextPair();
    )

#undef INTERFACES
