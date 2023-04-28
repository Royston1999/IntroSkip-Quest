#pragma once

#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"
#include "GlobalNamespace/VRController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "System/IDisposable.hpp"
#include "Zenject/DiContainer.hpp"
#include "Zenject/IInitializable.hpp"
#include "custom-types/shared/macros.hpp"
#include "lapiz/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
#include "GlobalNamespace/ComboUIController.hpp"
#include "GlobalNamespace/ComboController.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include <vector>
#include "Utils/IntroSkipUtils.hpp"
#include "Zenject/ITickable.hpp"

using namespace GlobalNamespace;

#define INTERFACES                                                        \
    {                                                                     \
        classof(System::IDisposable*), classof(Zenject::IInitializable*), classof(Zenject::ITickable*)\
    }

___DECLARE_TYPE_WRAPPER_INHERITANCE(IntroSkip, IntroSkipController, Il2CppTypeEnum::IL2CPP_TYPE_CLASS, Il2CppObject, "IntroSkip", INTERFACES, 0, nullptr,
                                    DECLARE_PRIVATE_FIELD(AudioTimeSyncController*, _audioTimeSyncController);
                                    DECLARE_PRIVATE_FIELD(TMPro::TextMeshProUGUI*, _skipText);
                                    DECLARE_PRIVATE_FIELD(IReadonlyBeatmapData*, _mapData);
                                    DECLARE_PRIVATE_FIELD(IDifficultyBeatmap*, _difficultyBeatmap);
                                    DECLARE_PRIVATE_FIELD(VRController*, _leftController);
                                    DECLARE_PRIVATE_FIELD(VRController*, _rightController);
                                    DECLARE_CTOR(ctor, AudioTimeSyncController* audioTimeSyncController, IReadonlyBeatmapData* readonlyBeatmapData, IDifficultyBeatmap* difficultyBeatmap, PauseMenuManager* pauseMenuManager);
                                    DECLARE_OVERRIDE_METHOD(void, Initialize, il2cpp_utils::il2cpp_type_check::MetadataGetter<&::Zenject::IInitializable::Initialize>::get());
                                    DECLARE_OVERRIDE_METHOD(void, Dispose, il2cpp_utils::il2cpp_type_check::MetadataGetter<&::System::IDisposable::Dispose>::get());
                                    DECLARE_OVERRIDE_METHOD(void, Tick, il2cpp_utils::il2cpp_type_check::MetadataGetter<&::Zenject::ITickable::Tick>::get());
                                    private:
                                        SkipTimePairs skipTimePairs;
                                        SkipTimePairs::iterator skipItr;
                                        bool modEnabled;
                                        float songLength, lTriggerVal, rTriggerVal, requiredHoldTime;
                                        float timeHeld;
                                        TMPro::TextMeshProUGUI* CreateSkipText();
                                        void setSkipText(bool value);
                                        void iterateToNextPair();
                                    )

#undef INTERFACES
