#pragma once

#define INTRO_SKIP_EXPORT __attribute__((visibility("default")))
#ifdef __cplusplus
#define INTRO_SKIP_EXPORT_FUNC extern "C" INTRO_SKIP_EXPORT
#else
#define INTRO_SKIP_EXPORT_FUNC INTRO_SKIP_EXPORT
#endif

// Include the modloader header, which allows us to tell the modloader which mod this is, and the version etc.
#include "scotland2/shared/loader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

// Define these functions here so that we can easily read configuration and log information from other files
Configuration& getConfig();
const Paper::ConstLoggerContext<sizeof(MOD_ID)>& getLogger();