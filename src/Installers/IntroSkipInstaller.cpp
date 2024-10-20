#include "Installers/IntroSkipInstaller.hpp"

#include "IntroSkipController.hpp"
#include "Zenject/DiContainer.hpp"
#include "Zenject/FromBinderNonGeneric.hpp"
#include "Config.hpp"

DEFINE_TYPE(IntroSkip::Installers, IntroSkipInstaller);
namespace IntroSkip::Installers
{
    void IntroSkipInstaller::InstallBindings()
    {
        if (!getIntroSkipConfig().isEnabled.GetValue()) return;
        get_Container()->BindInterfacesAndSelfTo<IntroSkip::IntroSkipController*>()->AsSingle();
    }
}
