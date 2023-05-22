#include "Installers/IntroSkipInstaller.hpp"

#include "IntroSkipController.hpp"
#include "Zenject/DiContainer.hpp"
#include "Zenject/FromBinderNonGeneric.hpp"
#include "lapiz/shared/utilities/ZenjectExtensions.hpp"
#include "Config.hpp"

DEFINE_TYPE(IntroSkip::Installers, IntroSkipInstaller);

using namespace Lapiz::Zenject::ZenjectExtensions;

namespace IntroSkip::Installers
{
    void IntroSkipInstaller::InstallBindings()
    {
        if (!getIntroSkipConfig().isEnabled.GetValue()) return;
        get_Container()->BindInterfacesAndSelfTo<IntroSkip::IntroSkipController*>()->AsSingle();
    }
}
