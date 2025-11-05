#include "pch.h"

#include "Component/Public/AudioComponent.h"
#include "Audio/Public/SoundWave.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include <json.hpp>

IMPLEMENT_CLASS(UAudioComponent, UActorComponent)

UAudioComponent::UAudioComponent()
    : Sound(nullptr)
    , bLooping(false)
    , ActiveSound(nullptr)
{
}

void UAudioComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FString FileName;
		FJsonSerializer::ReadString(InOutHandle, "SoundWaveAsset", FileName);
		LoadSound(FileName);

		FJsonSerializer::ReadBool(InOutHandle, "Looping", bLooping);
	}
	else
	{
		if (Sound)
		{
			InOutHandle["SoundWaveAsset"] = Sound->FileName;
		}
		InOutHandle["Looping"] = bLooping;
	}
}

UObject* UAudioComponent::Duplicate()
{
	UAudioComponent* AudioComponent = Cast<UAudioComponent>(Super::Duplicate());

    AudioComponent->Sound = Sound;
    AudioComponent->bLooping = bLooping;

    return AudioComponent;
}

void UAudioComponent::Play()
{
    if (GWorld && GWorld->GetAudioDevice())
    {
        GWorld->GetAudioDevice()->PlaySound(this); 
    }
}

void UAudioComponent::Stop()
{
    if (GWorld && GWorld->GetAudioDevice())
    {
        GWorld->GetAudioDevice()->StopSound(this);
    }
}

void UAudioComponent::LoadSound(const FString& InFileName)
{
    Sound = UAssetManager::GetInstance().LoadSound(InFileName);
}
