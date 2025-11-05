#pragma once

#include <xaudio2.h>

#include "SoundWave.h"

class UAudioComponent;

class FActiveSound
{
public:
    FActiveSound()
        : SourceVoice(nullptr)
        , AudioComponent(nullptr)
        , SoundWave(nullptr)
    {
    }

    IXAudio2SourceVoice* SourceVoice;
    UAudioComponent* AudioComponent;
    USoundWave* SoundWave;
};

class FAudioDevice
{
public:
    FAudioDevice();
    FAudioDevice(const FAudioDevice& Other) = delete;
    FAudioDevice(FAudioDevice&& Other) = delete;
    FAudioDevice& operator=(const FAudioDevice& Other) = delete;
    FAudioDevice& operator=(FAudioDevice&& Other) = delete;
    ~FAudioDevice();

    bool Initialize();

    void Shutdown();

    void Update(float DeltaTime);

    /** @todo: FActiveSound를 반환 */
    FActiveSound* PlaySound(UAudioComponent* InComponent);

    void StopSound(UAudioComponent* InComponent);

private:
    FActiveSound* FindActiveSound(UAudioComponent* InComponent);

    void RemoveAndDestroyActiveSound(FActiveSound* ActiveSound);
    
    IXAudio2* XAudio2;

    IXAudio2MasteringVoice* MasteringVoice;

    TArray<FActiveSound*> ActiveSounds;
};
