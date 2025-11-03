#pragma once

#include "Enemy.h"

class AEightBallEnemy : public AEnemy
{
    GENERATED_BODY()
    DECLARE_CLASS(AEightBallEnemy, AEnemy)
public:
    AEightBallEnemy() = default;

    virtual ~AEightBallEnemy() = default;

    virtual void InitializeComponents() override;
};
