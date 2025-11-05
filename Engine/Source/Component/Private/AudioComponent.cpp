#include "pch.h"

#include "Component/Public/AudioComponent.h"

IMPLEMENT_CLASS(UAudioComponent, UActorComponent)

UAudioComponent::UAudioComponent()
    : Sound(nullptr)
    , bLooping(false)
    , ActiveSound(nullptr)
{
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
