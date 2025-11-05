#include "pch.h"
#include "Editor/Public/CameraShakePresetDetailPanel.h"
#include "ImGui/imgui.h"
#include "Global/Public/BezierCurve.h"
#include <cstring>

FCameraShakePresetDetailPanel::FCameraShakePresetDetailPanel()
	: bShowPreview(true)
	, PreviewSampleX(0.5f)
	, TempDuration(1.0f)
	, TempLocationAmplitude(5.0f)
	, TempRotationAmplitude(2.0f)
	, TempPattern(0)
	, TempFrequency(10.0f)
	, bTempUseDecayCurve(false)
	, PresetButtonWidth(80.0f)
	, PreviewGraphHeight(100.0f)
{
	memset(TempPresetName, 0, sizeof(TempPresetName));
}

FCameraShakePresetDetailPanel::~FCameraShakePresetDetailPanel()
{
}

bool FCameraShakePresetDetailPanel::Draw(const char* Label, FCameraShakePresetData* Preset)
{
	if (!Preset)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No Preset Selected");
		return false;
	}

	bool bChanged = false;

	// 임시 버퍼에 현재 값 복사
	strncpy_s(TempPresetName, Preset->PresetName.ToString().c_str(), sizeof(TempPresetName) - 1);
	TempDuration = Preset->Duration;
	TempLocationAmplitude = Preset->LocationAmplitude;
	TempRotationAmplitude = Preset->RotationAmplitude;
	TempPattern = static_cast<int32>(Preset->Pattern);
	TempFrequency = Preset->Frequency;
	bTempUseDecayCurve = Preset->bUseDecayCurve;

	ImGui::PushID(Label);

	// ===== Preset Name =====
	if (DrawPresetName(Preset))
		bChanged = true;

	ImGui::Separator();

	// ===== Shake Parameters =====
	if (DrawShakeParameters(Preset))
		bChanged = true;

	ImGui::Separator();

	// ===== Bezier Curve Editor =====
	if (DrawBezierEditor(Preset))
		bChanged = true;

	// ===== Preset Buttons =====
	if (DrawPresetButtons(Preset))
		bChanged = true;

	// ===== Preview Graph =====
	ImGui::Checkbox("Show Preview", &bShowPreview);
	if (bShowPreview)
	{
		DrawPreviewGraph(Preset);
	}

	ImGui::PopID();

	return bChanged;
}

bool FCameraShakePresetDetailPanel::DrawPresetName(FCameraShakePresetData* Preset)
{
	bool bChanged = false;

	ImGui::Text("Preset Name:");
	ImGui::SetNextItemWidth(-1);
	if (ImGui::InputText("##PresetName", TempPresetName, sizeof(TempPresetName)))
	{
		Preset->PresetName = FName(TempPresetName);
		bChanged = true;
	}

	return bChanged;
}

bool FCameraShakePresetDetailPanel::DrawBezierEditor(FCameraShakePresetData* Preset)
{
	bool bChanged = false;

	ImGui::Text("Decay Curve Editor:");

	// Use Decay Curve 체크박스
	if (ImGui::Checkbox("Use Bezier Decay Curve", &bTempUseDecayCurve))
	{
		Preset->bUseDecayCurve = bTempUseDecayCurve;
		bChanged = true;
	}

	if (!Preset->bUseDecayCurve)
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Using default linear decay");
		return bChanged;
	}

	// Bezier Editor
	ImGui::BeginChild("BezierEditorRegion", ImVec2(0, 300), true);

	FCubicBezierCurve& Curve = Preset->DecayCurve;
	if (BezierEditor.Edit("BezierEditor", Curve, ImVec2(250, 250)))
	{
		bChanged = true;
	}

	ImGui::EndChild();

	return bChanged;
}

bool FCameraShakePresetDetailPanel::DrawPresetButtons(FCameraShakePresetData* Preset)
{
	bool bChanged = false;

	if (!Preset->bUseDecayCurve)
		return false;

	ImGui::Text("Curve Presets:");

	// Linear
	if (ImGui::Button("Linear", ImVec2(PresetButtonWidth, 0)))
	{
		Preset->DecayCurve = FCubicBezierCurve::CreateLinear();
		bChanged = true;
	}
	ImGui::SameLine();

	// EaseIn
	if (ImGui::Button("EaseIn", ImVec2(PresetButtonWidth, 0)))
	{
		Preset->DecayCurve = FCubicBezierCurve::CreateEaseIn();
		bChanged = true;
	}
	ImGui::SameLine();

	// EaseOut
	if (ImGui::Button("EaseOut", ImVec2(PresetButtonWidth, 0)))
	{
		Preset->DecayCurve = FCubicBezierCurve::CreateEaseOut();
		bChanged = true;
	}
	ImGui::SameLine();

	// EaseInOut
	if (ImGui::Button("EaseInOut", ImVec2(PresetButtonWidth, 0)))
	{
		Preset->DecayCurve = FCubicBezierCurve::CreateEaseInOut();
		bChanged = true;
	}

	return bChanged;
}

void FCameraShakePresetDetailPanel::DrawPreviewGraph(FCameraShakePresetData* Preset)
{
	if (!Preset->bUseDecayCurve)
		return;

	ImGui::BeginChild("PreviewGraphRegion", ImVec2(0, PreviewGraphHeight), true);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize = ImGui::GetContentRegionAvail();

	if (CanvasSize.x < 50.0f || CanvasSize.y < 50.0f)
	{
		ImGui::EndChild();
		return;
	}

	// 배경
	DrawList->AddRectFilled(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
		IM_COL32(30, 30, 30, 255));

	// 그리드
	const int32 GridSteps = 10;
	for (int32 i = 0; i <= GridSteps; ++i)
	{
		float t = static_cast<float>(i) / GridSteps;
		float x = CanvasPos.x + t * CanvasSize.x;
		float y = CanvasPos.y + t * CanvasSize.y;

		// 세로선
		DrawList->AddLine(ImVec2(x, CanvasPos.y), ImVec2(x, CanvasPos.y + CanvasSize.y),
			IM_COL32(50, 50, 50, 255), 1.0f);

		// 가로선
		DrawList->AddLine(ImVec2(CanvasPos.x, y), ImVec2(CanvasPos.x + CanvasSize.x, y),
			IM_COL32(50, 50, 50, 255), 1.0f);
	}

	// Bezier 곡선 그리기
	const int32 Steps = 100;
	for (int32 i = 0; i < Steps; ++i)
	{
		float t0 = static_cast<float>(i) / Steps;
		float t1 = static_cast<float>(i + 1) / Steps;

		float y0 = Preset->DecayCurve.Evaluate(t0).Y;
		float y1 = Preset->DecayCurve.Evaluate(t1).Y;

		ImVec2 p0(CanvasPos.x + t0 * CanvasSize.x, CanvasPos.y + (1.0f - y0) * CanvasSize.y);
		ImVec2 p1(CanvasPos.x + t1 * CanvasSize.x, CanvasPos.y + (1.0f - y1) * CanvasSize.y);

		DrawList->AddLine(p0, p1, IM_COL32(0, 255, 0, 255), 2.0f);
	}

	// 샘플 포인트
	float SampleY = Preset->DecayCurve.Evaluate(PreviewSampleX).Y;
	ImVec2 SamplePoint(
		CanvasPos.x + PreviewSampleX * CanvasSize.x,
		CanvasPos.y + (1.0f - SampleY) * CanvasSize.y
	);
	DrawList->AddCircleFilled(SamplePoint, 5.0f, IM_COL32(255, 255, 0, 255));

	ImGui::EndChild();

	// 샘플 X 슬라이더
	ImGui::Text("Preview Sample X:");
	ImGui::SliderFloat("##PreviewX", &PreviewSampleX, 0.0f, 1.0f, "%.3f");

	// 샘플 값 표시
	ImGui::Text("Sample Value: %.3f", SampleY);
}

bool FCameraShakePresetDetailPanel::DrawShakeParameters(FCameraShakePresetData* Preset)
{
	bool bChanged = false;

	ImGui::Text("Shake Parameters:");

	// Duration
	ImGui::Text("Duration (sec):");
	ImGui::SetNextItemWidth(-1);
	if (ImGui::DragFloat("##Duration", &TempDuration, 0.1f, 0.1f, 60.0f, "%.2f"))
	{
		Preset->Duration = TempDuration;
		bChanged = true;
	}

	// Location Amplitude
	ImGui::Text("Location Amplitude:");
	ImGui::SetNextItemWidth(-1);
	if (ImGui::DragFloat("##LocationAmp", &TempLocationAmplitude, 1.0f, 0.0f, 200.0f, "%.1f"))
	{
		Preset->LocationAmplitude = TempLocationAmplitude;
		bChanged = true;
	}

	// Rotation Amplitude
	ImGui::Text("Rotation Amplitude (deg):");
	ImGui::SetNextItemWidth(-1);
	if (ImGui::DragFloat("##RotationAmp", &TempRotationAmplitude, 0.5f, 0.0f, 45.0f, "%.1f"))
	{
		Preset->RotationAmplitude = TempRotationAmplitude;
		bChanged = true;
	}

	// Pattern
	ImGui::Text("Shake Pattern:");
	const char* PatternNames[] = { "Sine", "Perlin", "Random" };
	ImGui::SetNextItemWidth(-1);
	if (ImGui::Combo("##Pattern", &TempPattern, PatternNames, IM_ARRAYSIZE(PatternNames)))
	{
		Preset->Pattern = static_cast<ECameraShakePattern>(TempPattern);
		bChanged = true;
	}

	// Frequency
	ImGui::Text("Frequency (Hz):");
	ImGui::SetNextItemWidth(-1);
	if (ImGui::DragFloat("##Frequency", &TempFrequency, 0.5f, 0.1f, 100.0f, "%.1f"))
	{
		Preset->Frequency = TempFrequency;
		bChanged = true;
	}

	return bChanged;
}
