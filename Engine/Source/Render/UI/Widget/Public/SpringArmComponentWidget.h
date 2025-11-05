#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class USpringArmComponent;

UCLASS()
class USpringArmComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(USpringArmComponentWidget, UWidget)

public:
	void Initialize() override {}
	void Update() override;
	void RenderWidget() override;

private:
	USpringArmComponent* Component{};
};
