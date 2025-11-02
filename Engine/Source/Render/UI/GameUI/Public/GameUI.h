#pragma once
#include "Core/Public/Object.h"

//임시로 빠르게 UI제작을 위해 사용
//파이 모드에서만 동작
//Button(게임시작), Text(데미지, 스코어), 게이지바(체력)


UCLASS()
class UGameUI : public UObject
{
	GENERATED_BODY()
	DECLARE_SINGLETON_CLASS(UGameUI, UObject)

public:
	void TextUI(const FString& Text, const FVector2& ScreenPos, float Size, const FVector4& InColor = FVector4(1, 1, 1, 1));
	void HPBar(const FVector2& ScreenPos, const FVector2& Size, float HPPer);


};
