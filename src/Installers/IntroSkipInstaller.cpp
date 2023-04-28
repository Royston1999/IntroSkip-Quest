#include "Installers/IntroSkipInstaller.hpp"

#include "IntroSkipController.hpp"
#include "Zenject/ConcreteBinderGeneric_1.hpp"
#include "Zenject/ConcreteIdBinderGeneric_1.hpp"
#include "Zenject/DiContainer.hpp"
#include "Zenject/FromBinderNonGeneric.hpp"
#include "lapiz/shared/utilities/ZenjectExtensions.hpp"

DEFINE_TYPE(IntroSkip::Installers, IntroSkipInstaller);

using namespace Lapiz::Zenject::ZenjectExtensions;

namespace IntroSkip::Installers
{
    void IntroSkipInstaller::InstallBindings()
    {
        get_Container()->BindInterfacesAndSelfTo<IntroSkip::IntroSkipController*>()->AsSingle();
    }
}
