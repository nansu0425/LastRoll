#pragma once

#include "PostProcessPass.h"

class FGammaPass : public FPostProcessPass
{
public:
    FGammaPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FGammaPass() = default;

    virtual bool IsEnabled(FRenderingContext& Context) const override;
};
