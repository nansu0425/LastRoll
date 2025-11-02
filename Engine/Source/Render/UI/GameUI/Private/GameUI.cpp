#include "pch.h"
#include "Render/UI/GameUI/Public/GameUI.h"
#include "Render/UI/Overlay/Public/D2DOverlayManager.h"

IMPLEMENT_SINGLETON_CLASS(UGameUI, UObject)

UGameUI::UGameUI()
{

}
UGameUI::~UGameUI()
{

}

wstring ToWString(const FString& InStr)
{
    if (InStr.empty())
    {
        return {};
    }

    int SizeNeeded = MultiByteToWideChar(CP_UTF8, 0, InStr.c_str(), static_cast<int>(InStr.size()), nullptr, 0);
    wstring WideStr(SizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, InStr.c_str(), static_cast<int>(InStr.size()), WideStr.data(), SizeNeeded);
    return WideStr;
}

void UGameUI::TextUI(const FString& Text, const FVector2& ScreenPos, float Size, const FVector4& InColor)
{
    wstring WideText = ToWString(Text);
    D2D1_RECT_F Rect;
    Rect.top = ScreenPos.Y + 50;
    Rect.bottom = ScreenPos.Y - 50;
    Rect.left = ScreenPos.X - 250;
    Rect.right = ScreenPos.X + 250;

    D2D1_COLOR_F Color;
    Color.r = InColor.X;
    Color.g = InColor.Y;
    Color.b = InColor.Z;
    Color.a = InColor.W;
    FD2DOverlayManager::GetInstance().AddText(WideText.c_str(), Rect, Color, Size);
}
void UGameUI::HPBar(const FVector2& ScreenPos, const FVector2& Size, float HPPer)
{

}