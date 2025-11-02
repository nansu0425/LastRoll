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

void UGameUI::TextUI(const FString& Text, const FVector2& ScreenPos, const FVector2& RectSize, float Size, const FVector4& InColor)
{
    wstring WideText = ToWString(Text);
    D2D1_RECT_F Rect;
    Rect.top = ScreenPos.Y + RectSize.Y * 0.5f;
    Rect.bottom = ScreenPos.Y - RectSize.Y * 0.5f;
    Rect.left = ScreenPos.X - RectSize.X * 0.5f;
    Rect.right = ScreenPos.X + RectSize.X * 0.5f;

    D2D1_COLOR_F Color;
    Color.r = InColor.X;
    Color.g = InColor.Y;
    Color.b = InColor.Z;
    Color.a = InColor.W;
    FD2DOverlayManager::GetInstance().AddText(WideText.c_str(), Rect, Color, Size);
}
void UGameUI::HPBar(const FVector2& ScreenPos, const FVector2& Size, float HPPer)
{
    D2D1_COLOR_F BGColor;
    BGColor.r = 0.2f;
    BGColor.g = 0.2f;
    BGColor.b = 0.2f;
    BGColor.a = 1.0f;

    D2D1_COLOR_F GaugeColor;
    GaugeColor.r = 1.0f;
    GaugeColor.g = 0.2f;
    GaugeColor.b = 0.2f;
    GaugeColor.a = 1.0f;

    D2D1_RECT_F BGRect;
    BGRect.top = ScreenPos.Y + Size.Y * 0.5f;
    BGRect.bottom = ScreenPos.Y - Size.Y * 0.5f;
    BGRect.left = ScreenPos.X - Size.X * 0.5f;
    BGRect.right = ScreenPos.X + Size.X * 0.5f;

    D2D1_RECT_F GaugeRect;
    GaugeRect.top = BGRect.top - 5;
    GaugeRect.bottom = BGRect.bottom + 5;
    GaugeRect.left = BGRect.left + 5;
    float GaugeMaxWidth = BGRect.right - BGRect.left - 10;
    GaugeRect.right = GaugeRect.left + GaugeMaxWidth * HPPer;

    FD2DOverlayManager::GetInstance().AddRectangle(BGRect, BGColor);
    FD2DOverlayManager::GetInstance().AddRectangle(GaugeRect, GaugeColor);
}