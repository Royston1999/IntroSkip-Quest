#pragma once
#include "beatsaber-hook/shared/utils/logging.hpp"
#include <string_view>

namespace IntroSkip
{
    class Logging 
    {
        public:
            static Logger& getLogger();
            static LoggerContextObject& getContextLogger(const char* fun, const char* file, int line);
    };
}
#define INFO(...) ::IntroSkip::Logging::getContextLogger(__PRETTY_FUNCTION__, __FILE__, __LINE__).info(__VA_ARGS__)
#define ERROR(...) ::IntroSkip::Logging::getContextLogger(__PRETTY_FUNCTION__, __FILE__, __LINE__).error(__VA_ARGS__)
#define CRITICAL(...) ::IntroSkip::Logging::getContextLogger(__PRETTY_FUNCTION__, __FILE__, __LINE__).critical(__VA_ARGS__)