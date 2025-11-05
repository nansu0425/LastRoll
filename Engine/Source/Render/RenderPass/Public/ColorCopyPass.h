#pragma once

#include "PostProcessPass.h"

class FColorCopyPass : public FPostProcessPass
{
public:
    FColorCopyPass(UPipeline* InPipeline, UDeviceResources* InDeviceResources);

    virtual ~FColorCopyPass();

protected:
private:
};
