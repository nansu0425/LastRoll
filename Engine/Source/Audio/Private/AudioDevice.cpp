#include "pch.h"
#include "Audio/Public/AudioDevice.h"
#include "Component/Public/AudioComponent.h"

FAudioDevice::FAudioDevice()
    : XAudio2(nullptr)
    , MasteringVoice(nullptr)
{
}

FAudioDevice::~FAudioDevice()
{
    Shutdown();
}

bool FAudioDevice::Initialize()
{
    HRESULT HResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(HResult)) return false;

    XAudio2 = nullptr;
    HResult = XAudio2Create(&XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(HResult))
    {
        CoUninitialize();
        return false;
    }

    MasteringVoice = nullptr;
    HResult = XAudio2->CreateMasteringVoice(&MasteringVoice);
    if (FAILED(HResult))
    {
        XAudio2->Release();
        CoUninitialize();
        return false;
    }

    return true;
}

void FAudioDevice::Shutdown()
{
    while (!ActiveSounds.empty())
    {
        RemoveAndDestroyActiveSound(ActiveSounds.back());     
    }
    
    if (MasteringVoice)
    {
        MasteringVoice->DestroyVoice();
        MasteringVoice = nullptr;
    }
    if (XAudio2)
    {
        XAudio2->Release();
        XAudio2 = nullptr;
    }
    CoUninitialize();
}

void FAudioDevice::Update(float DeltaTime)
{
    for (int32 i = static_cast<int32>(ActiveSounds.size() - 1); i >= 0; --i)
    {
        FActiveSound* ActiveSound = ActiveSounds[i];

        if (ActiveSound->AudioComponent && ActiveSound->AudioComponent->bLooping)
        {
            continue; 
        }

        XAUDIO2_VOICE_STATE State;
        ActiveSound->SourceVoice->GetState(&State);

        if (State.BuffersQueued == 0)
        {
            RemoveAndDestroyActiveSound(ActiveSound);
        }
    }
}

FActiveSound* FAudioDevice::PlaySound(UAudioComponent* InComponent)
{
    if (!InComponent || !InComponent->Sound || !InComponent->Sound->WaveData)
    {
        return nullptr;
    }

    FActiveSound* ExistingSound = FindActiveSound(InComponent);
    if (ExistingSound)
    {
        StopSound(InComponent);
    }
    
    USoundWave* Wave = InComponent->Sound;

    FActiveSound* NewActiveSound = new FActiveSound();
    NewActiveSound->AudioComponent = InComponent;
    NewActiveSound->SoundWave = Wave;

    HRESULT HResult = XAudio2->CreateSourceVoice(
        &NewActiveSound->SourceVoice,
        &(Wave->WaveFormat)
    );
    if (FAILED(HResult))
    {
        delete NewActiveSound;
        return nullptr;
    }

    XAUDIO2_BUFFER Buffer = { 0 };
    Buffer.pAudioData = Wave->WaveData;
    Buffer.Flags = XAUDIO2_END_OF_STREAM; 
    Buffer.AudioBytes = Wave->WaveDataSize;

    if (InComponent->bLooping) 
    {
        Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    }
    else
    {
        Buffer.LoopCount = 0; 
    }

    HResult = NewActiveSound->SourceVoice->SubmitSourceBuffer(&Buffer);
    if (FAILED(HResult))
    {
        NewActiveSound->SourceVoice->DestroyVoice();
        delete NewActiveSound;
        return nullptr;
    }

    HResult = NewActiveSound->SourceVoice->Start(0);
    if (FAILED(HResult))
    {
        UE_LOG_ERROR("사운드 재생에 실패했습니다.");
        return nullptr;
    }

    ActiveSounds.push_back(NewActiveSound);
    
    InComponent->ActiveSound = NewActiveSound; 

    return NewActiveSound;
}

void FAudioDevice::StopSound(UAudioComponent* InComponent)
{
    FActiveSound* Sound = FindActiveSound(InComponent);
    if (Sound)
    {
        RemoveAndDestroyActiveSound(Sound);
    }
}

FActiveSound* FAudioDevice::FindActiveSound(UAudioComponent* InComponent)
{
    for (FActiveSound* Sound : ActiveSounds)
    {
        if (Sound->AudioComponent == InComponent)
        {
            return Sound;
        }
    }
    return nullptr;
}

void FAudioDevice::RemoveAndDestroyActiveSound(FActiveSound* ActiveSound)
{
    if (!ActiveSound) return;

    ActiveSound->SourceVoice->Stop(0);
    ActiveSound->SourceVoice->FlushSourceBuffers(); 
    ActiveSound->SourceVoice->DestroyVoice(); 

    if (ActiveSound->AudioComponent)
    {
        ActiveSound->AudioComponent->ActiveSound = nullptr;
    }

    for (size_t i = 0; i < ActiveSounds.size(); ++i)
    {
        if (ActiveSounds[i] == ActiveSound)
        {
            ActiveSounds.erase(ActiveSounds.begin() + i);
            break;
        }
    }

    delete ActiveSound;
}
