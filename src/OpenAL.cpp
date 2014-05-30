#include "OpenAL.h"

// Specify storage for the OpenAL global variables
namespace OpenAL
{
    ALCdevice*           g_pAlDevice;
    ALCcontext*          g_pAlContext;
    std::deque<ALuint>   g_sources;
    std::deque<ALuint>   g_buffers;
} // namespace OpenAL
