#pragma once
#include "Audio/Public/AudioDevice.h"

class USoundWave;

UCLASS()
class UAudioComponent : public UActorComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UAudioComponent, UActorComponent)
public:
    UAudioComponent();

    virtual ~UAudioComponent() = default;

    void Play();

    void Stop();

    USoundWave* Sound;

    bool bLooping;

    FActiveSound* ActiveSound;
};
