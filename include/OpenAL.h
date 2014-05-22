#pragma once

#include "AL/al.h"
#include "AL/alc.h"

class OpenAL
{
 private:
    static ALCdevice* GetDevice()
    {
        static ALCdevice* pAlDevice;
        return pAlDevice;
    };

    static ALCcontext* GetContext()
    {
        static ALCcontext* pAlContext;
        return pAlContext;
    };

    struct RIFF_Header {
        char chunkID[4];
        long chunkSize;         // Size not including chunkSize or chunkID
        char format[4];
    };

    struct WAVE_Format {
        char subChunkID[4];
        long subChunkSize;
        short audioFormat;
        short numChannels;
        long sampleRate;
        long byteRate;
        short blockAlign;
        short bitsPerSample;
    };

    struct WAVE_Data {
        char subChunkID[4];     // Should contain the word data
        long subChunk2Size;     // Stores the size of the data block
    };

public:
    static void InitOpenAL()
    {
        ALCdevice*  pAlDevice  = GetDevice();
        ALCcontext* pAlContext = GetContext();

        try
        {
            pAlDevice = alcOpenDevice(NULL);
            if (pAlDevice == NULL)
                throw ("Error occurred creating AL device");
            pAlContext = alcCreateContext(pAlDevice, NULL);
            if (pAlContext == NULL)
                throw ("Error occurred creating AL context");
            alcMakeContextCurrent(pAlContext);
            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred initializing OpenAL");
        }
        catch(std::string error) 
        {
            // Our catch statement for if we throw a string
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
        ALCdevice*  pAlDevice  = GetDevice();
        ALCcontext* pAlContext = GetContext();

        alcMakeContextCurrent(NULL);
        alcDestroyContext(pAlContext);
        alcCloseDevice(pAlDevice);
    }

    static ALuint CreateBuffer(ci::DataSourceRef ref)
    {
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
            // Our catch statement for if we throw a string
            std::cerr << error << " : trying to load "
                << ref->getFilePath() << std::endl;
            return 0;
        }
        return alBuffer;
    }

    static ALuint CreateSource(ALuint alBuffer)
    {
        ALuint          alSource;
        try
        {
            if (alGetError() != AL_NO_ERROR)
                throw ("Error occurred before creating source");

            // Create our openAL source and check for success
            alGenSources(1, &alSource);
            if (alGetError() != AL_NO_ERROR)
                throw ("alGenSources threw an error");

            ALfloat sourcePos[] = { 0.0, 0.0, 0.0 };
            ALfloat sourceVel[] = { 0.0, 0.0, 0.0 };

            alSourcei (alSource, AL_BUFFER,   alBuffer);   
            alSourcef (alSource, AL_PITCH,    1.0f);
            alSourcef (alSource, AL_GAIN,     1.0f);
            alSourcefv(alSource, AL_POSITION, sourcePos);
            alSourcefv(alSource, AL_VELOCITY, sourceVel);
            alSourcei (alSource, AL_LOOPING,  AL_FALSE );
            if (alGetError() != AL_NO_ERROR)
                throw ("Error setting source parameters");
        }
        catch(std::string error) 
        {
            // Our catch statement for if we throw a string
            std::cerr << error << std::endl;
            return 0;
        }
        return alSource;
    }
};
