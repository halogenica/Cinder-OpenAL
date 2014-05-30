#pragma once

#include "AL/al.h"
#include "AL/alc.h"

#include "cinder/DataSource.h"
#include "cinder/Vector.h"

#include <iostream>
#include <sstream>
#include <deque>

namespace OpenAL
{

class Sound;

extern ALCdevice*           g_pAlDevice;
extern ALCcontext*          g_pAlContext;

// A list of sources unassociated with sounds from least to most recently used
extern std::deque<ALuint>   g_sources;

// A list of internally created buffers
extern std::deque<ALuint>   g_buffers;

static void InitOpenAL()
{
    try
    {
        g_pAlDevice = alcOpenDevice(NULL);
        if (g_pAlDevice == NULL)
            throw ("Error occurred creating AL device");
        g_pAlContext = alcCreateContext(g_pAlDevice, NULL);
        if (g_pAlContext == NULL)
            throw ("Error occurred creating AL context");
        alcMakeContextCurrent(g_pAlContext);
        if (alGetError() != AL_NO_ERROR)
            throw ("Error occurred initializing OpenAL");
    }
    catch(std::string error) 
    {
        std::cerr << error << std::endl;
    }

    ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };
    ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
    ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
    alListenerfv(AL_POSITION,    ListenerPos);
    alListenerfv(AL_VELOCITY,    ListenerVel);
    alListenerfv(AL_ORIENTATION, ListenerOri);
}

static void DestroyOpenAL()
{
    for (ALuint buffer : g_buffers)
    {
        alDeleteBuffers(1, &buffer);
    }

    for (ALuint source : g_sources)
    {
        alDeleteSources(1, &source);
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(g_pAlContext);
    alcCloseDevice(g_pAlDevice);
}

static void SetListenerPosition(const ci::Vec3f& position)
{
    ALfloat ListenerPos[] = { position.x, position.y, position.z };
    alListenerfv(AL_POSITION,    ListenerPos);
}

static void SetListenerVelocity(const ci::Vec3f& velocity)
{
    ALfloat ListenerVel[] = { velocity.x, velocity.y, velocity.z };
    alListenerfv(AL_VELOCITY,    ListenerVel);
}

static void SetListenerOrientation(const ci::Vec3f& forward, const ci::Vec3f& up)
{
    ALfloat ListenerOri[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    alListenerfv(AL_ORIENTATION, ListenerOri);
}

static void SetListenerGain(const float& gain)
{
    ALfloat listenerGain = gain;
    alListenerf(AL_GAIN, listenerGain);
}

// Optional interface call for apps that wish to reuse buffers
static ALuint CreateBuffer(const ci::DataSourceRef& ref)
{
    struct RIFF_Header
    {
        char chunkID[4];
        long chunkSize;         // Size not including chunkSize or chunkID
        char format[4];
    };

    struct WAVE_Format 
    {
        char subChunkID[4];
        long subChunkSize;
        short audioFormat;
        short numChannels;
        long sampleRate;
        long byteRate;
        short blockAlign;
        short bitsPerSample;
    };

    struct WAVE_Data 
    {
        char subChunkID[4];     // Should contain the word data
        long subChunk2Size;     // Stores the size of the data block
    };

    char*           pRefBuffer = static_cast<char*>(ref->getBuffer().getData());
    RIFF_Header*    pRiffHeader;
    WAVE_Format*    pWaveFormat;
    WAVE_Data*      pWaveData;
    size_t          ptrOffset = 0;
    ALuint          alBuffer;

    try
    {
        if (alGetError() != AL_NO_ERROR)
            throw ("Error occurred before loading wav");

        pRiffHeader = reinterpret_cast<RIFF_Header*>(pRefBuffer);
        ptrOffset += sizeof(RIFF_Header);
        // Check for RIFF and WAVE tag in memeory
        if ((pRiffHeader->chunkID[0] != 'R' ||
                pRiffHeader->chunkID[1] != 'I' ||
                pRiffHeader->chunkID[2] != 'F' ||
                pRiffHeader->chunkID[3] != 'F') ||
            (pRiffHeader->format[0] != 'W' ||
                pRiffHeader->format[1] != 'A' ||
                pRiffHeader->format[2] != 'V' ||
                pRiffHeader->format[3] != 'E'))
                    throw ("Invalid RIFF or WAVE Header");

        // Read in the 2nd chunk for the wave info
        pWaveFormat = (WAVE_Format*)(pRefBuffer + ptrOffset);
        ptrOffset += sizeof(WAVE_Format);
        // Check for fmt tag in memory
        if (pWaveFormat->subChunkID[0] != 'f' ||
            pWaveFormat->subChunkID[1] != 'm' ||
            pWaveFormat->subChunkID[2] != 't' ||
            pWaveFormat->subChunkID[3] != ' ')
                    throw ("Invalid Wave Format");

        // Check for extra parameters
        if (pWaveFormat->subChunkSize > 16)
        {
            ptrOffset += sizeof(short);
        }

        // Read in the the last byte of data before the sound file
        pWaveData = (WAVE_Data*)(pRefBuffer + ptrOffset);
        ptrOffset += sizeof(WAVE_Data);
        // Check for data tag in memory
        if (pWaveData->subChunkID[0] != 'd' ||
            pWaveData->subChunkID[1] != 'a' ||
            pWaveData->subChunkID[2] != 't' ||
            pWaveData->subChunkID[3] != 'a')
                    throw ("Invalid data header");

        ALsizei size = static_cast<ALsizei>(pWaveData->subChunk2Size);
        if (ref->getBuffer().getDataSize() != size + ptrOffset)
            throw ("Buffer size different than reported size");

        ALsizei frequency = static_cast<ALsizei>(pWaveFormat->sampleRate);
        ALenum  format;
        // The format is worked out by looking at the number of
        // Channels and the bits per sample.
        if (pWaveFormat->numChannels == 1) {
            if (pWaveFormat->bitsPerSample == 8 )
                format = AL_FORMAT_MONO8;
            else if (pWaveFormat->bitsPerSample == 16)
                format = AL_FORMAT_MONO16;
        } else if (pWaveFormat->numChannels == 2) {
            if (pWaveFormat->bitsPerSample == 8 )
                format = AL_FORMAT_STEREO8;
            else if (pWaveFormat->bitsPerSample == 16)
                format = AL_FORMAT_STEREO16;
        }

        // Create our openAL buffer and check for success
        alGenBuffers(1, &alBuffer);
        if (alGetError() != AL_NO_ERROR)
            throw ("alGenBuffers threw an error");
        // Now we put our data into the openAL buffer and check for success
        alBufferData(alBuffer, format, (void*)(pRefBuffer + ptrOffset), size, frequency);
        if (alGetError() != AL_NO_ERROR)
            throw ("alBufferData threw an error");
    }
    catch(std::string error) 
    {
        std::cerr << error << " : trying to load "
            << ref->getFilePath() << std::endl;
        return 0;
    }
    return alBuffer;
}

// TODO: allow users to create and manage their own sources


class Sound
{
public:
    float       m_pitch;
    float       m_gain;
    ci::Vec3f   m_position;
    ci::Vec3f   m_velocity;
    bool        m_looping;

    Sound(const ALuint& alBuffer) : 
        m_buffer(alBuffer), m_source(0), m_pitch(1.f), m_gain(1.f), m_position(ci::Vec3f(0.f, 0.f, 0.f)), m_velocity(ci::Vec3f(0.f, 0.f, 0.f)), m_looping(false)
    {
    }

    // Convenience function if not reusing buffer
    Sound(const ci::DataSourceRef& ref) : 
        m_buffer(0), m_source(0), m_pitch(1.f), m_gain(1.f), m_position(ci::Vec3f(0.f, 0.f, 0.f)), m_velocity(ci::Vec3f(0.f, 0.f, 0.f)), m_looping(false)
    {
        m_buffer = CreateBuffer(ref);
        g_buffers.push_back(m_buffer);
    }

    ~Sound()
    {
        if (m_source)
        {
            alSourceStop(m_source);
            g_sources.push_back(m_source);
        }
    }

    // Convenience function for playing an "overlapping" sound (instead of restarting the sound)
    void Play(bool overlap = true)
    {
        try
        {
            if (m_source == 0)
            {
                m_source = GetSource();
            }
            else
            {
                ALenum state;
                alGetSourcei(m_source, AL_SOURCE_STATE, &state);
                if (state == AL_PLAYING)
                {
                    if (overlap)
                    {
                        g_sources.push_back(m_source);
                        m_source = GetSource();
                    }
                }
            }

            alSourcePlay(m_source);

            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred playing OpenAL sound");
        }
        catch(std::string error) 
        {
            std::cerr << error << std::endl;
        }
    }

    void Stop()
    {
        try
        {
            if (m_source)
            {
                alSourceStop(m_source);
                g_sources.push_back(m_source);
            }
            else
            {
                throw ("Stopping a source that wasn't playing");
            }

            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred stopping OpenAL sound");
        }
        catch(std::string error) 
        {
            std::cerr << error << std::endl;
        }
    }

    void Pause()
    {
        try
        {
            if (m_source)
            {
                alSourcePause(m_source);
            }
            else
            {
                throw ("Pausing a source that wasn't playing");
            }

            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred stopping OpenAL sound");
        }
        catch(std::string error) 
        {
            std::cerr << error << std::endl;
        }
    }

private:
    ALuint              m_buffer;
    ALuint              m_source;   // most recently played source

    ALuint CreateSource()
    {
        ALuint alSource;
        try
        {
            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred before creating source");

            // Create our openAL source and check for success
            alGenSources(1, &alSource);
            if (alGetError() != AL_NO_ERROR)
                throw ("alGenSources threw an error");
        }
        catch(std::string error) 
        {
            std::cerr << error << std::endl;
            alSource = 0;
        }
        return alSource;
    }

    // reuses sources if possible, otherwise creates new sources
    ALuint GetSource()
    {
        ALuint alSource = 0;
        try
        {
            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred before getting source");

            bool sourceFound = false;
            int i = 0;

            for (ALuint source : g_sources)
            {
                ALint state;
                alGetSourcei(source, AL_SOURCE_STATE, &state);
                if (state == AL_INITIAL || state == AL_STOPPED)
                {
                    alSource = source;
                    sourceFound = true;
                    break;
                }
                i++;
            }

            if (sourceFound)
            {
                g_sources.erase(g_sources.begin() + i);
            }
            else
            {
                alSource = CreateSource();
            }
            
            if (alSource)
            {
                ALfloat sourcePos[] = { m_position.x, m_position.y, m_position.z };
                ALfloat sourceVel[] = { m_velocity.x, m_velocity.y, m_velocity.z };

                alSourcei (alSource, AL_BUFFER,   m_buffer);   
                alSourcef (alSource, AL_PITCH,    m_pitch);
                alSourcef (alSource, AL_GAIN,     m_gain);
                alSourcefv(alSource, AL_POSITION, sourcePos);
                alSourcefv(alSource, AL_VELOCITY, sourceVel);
                alSourcei (alSource, AL_LOOPING,  m_looping );
                if (alGetError() != AL_NO_ERROR)
                    throw ("Error setting source parameters");
            }
        }
        catch(std::string error) 
        {
            std::cerr << error << std::endl;
            alSource = 0;
        }
        return alSource;
    }
};

};  // namespace OpenAL
