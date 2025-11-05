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

    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    virtual UObject* Duplicate() override;

    void Play();

    void Stop();

    void LoadSound(const FString& InFileName);

    USoundWave* Sound;

    bool bLooping;

    FActiveSound* ActiveSound;
};
