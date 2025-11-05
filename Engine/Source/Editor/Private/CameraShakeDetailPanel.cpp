#include "pch.h"
#include "Editor/Public/CameraShakeDetailPanel.h"
#include "ImGui/imgui.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Level/Public/World.h"

FCameraShakeDetailPanel::FCameraShakeDetailPanel()
	: bShowPreview(true)
	, PreviewSampleX(0.0f)
	, TempDuration(1.0f)
	, TempLocationAmplitude(5.0f)
	, TempRotationAmplitude(2.0f)
	, TempPattern(static_cast<int32>(ECameraShakePattern::Perlin))
	, TempFrequency(10.0f)
	, bTempUseDecayCurve(false)
	, PresetButtonWidth(80.0f)
	, PreviewGraphHeight(100.0f)
{
}

FCameraShakeDetailPanel::~FCameraShakeDetailPanel()
{
}

bool FCameraShakeDetailPanel::Draw(const char* Label, UCameraModifier_CameraShake* CameraShake, UWorld* World)
{
	if (!CameraShake)
		return false;

	bool bChanged = false;

	ImGui::PushID(Label);

	// ===== 타이틀 =====
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Camera Shake Detail");
	ImGui::Separator();

	// ===== PIE 모드 체크 =====
	bool bIsPIEMode = World && (World->GetWorldType() == EWorldType::PIE);
	if (!bIsPIEMode)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Camera Shake only works in PIE mode");
		ImGui::Text("Press the Play button to test Camera Shake");
		ImGui::Separator();
	}

	// ===== Bezier 곡선 사용 여부 토글 =====
	bool bUseCurve = CameraShake->IsUsingDecayCurve();
	if (ImGui::Checkbox("Use Bezier Curve Decay", &bUseCurve))
	{
		CameraShake->SetUseDecayCurve(bUseCurve);
		bChanged = true;
	}
	ImGui::Separator();

	// Bezier 곡선이 활성화된 경우에만 에디터 표시
	if (bUseCurve)
	{
		// ===== Bezier 에디터 섹션 =====
		if (DrawBezierEditor(CameraShake))
		{
			bChanged = true;
		}

		ImGui::Separator();

		// ===== Preset 버튼 섹션 =====
		if (DrawPresetButtons(CameraShake))
		{
			bChanged = true;
		}

		ImGui::Separator();

		// ===== 곡선 프리뷰 =====
		ImGui::Checkbox("Show Preview", &bShowPreview);
		if (bShowPreview)
		{
			DrawCurvePreview(CameraShake);
		}

		ImGui::Separator();
	}
	else
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Using default smoothstep decay");
		ImGui::Separator();
	}

	// ===== 흔들림 파라미터 =====
	if (DrawShakeParameters(CameraShake))
	{
		bChanged = true;
	}

	ImGui::Separator();

	// ===== 테스트 컨트롤 =====
	DrawTestControls(CameraShake, World);

	ImGui::PopID();

	return bChanged;
}

bool FCameraShakeDetailPanel::DrawBezierEditor(UCameraModifier_CameraShake* CameraShake)
{
	ImGui::Text("Decay Curve Editor");
	ImGui::Text("Drag control points to adjust decay curve");
	ImGui::Spacing();

	// 현재 곡선 가져오기 (복사본)
	FCubicBezierCurve Curve = CameraShake->GetDecayCurve();

	// Bezier 에디터 렌더링
	bool bCurveChanged = BezierEditor.Edit("DecayCurve", Curve, ImVec2(250, 250));

	if (bCurveChanged)
	{
		// 변경된 곡선 적용
		CameraShake->SetDecayCurve(Curve);
	}

	return bCurveChanged;
}

bool FCameraShakeDetailPanel::DrawPresetButtons(UCameraModifier_CameraShake* CameraShake)
{
	ImGui::Text("Presets:");
	ImGui::Spacing();

	bool bPresetSelected = false;

	// 5개의 preset 버튼 (2줄로 배치: 3개 + 2개)
	if (ImGui::Button("Linear", ImVec2(PresetButtonWidth, 0)))
	{
		CameraShake->SetDecayCurve(FCubicBezierCurve::CreateLinear());
		bPresetSelected = true;
	}
	ImGui::SameLine();

	if (ImGui::Button("EaseIn", ImVec2(PresetButtonWidth, 0)))
	{
		CameraShake->SetDecayCurve(FCubicBezierCurve::CreateEaseIn());
		bPresetSelected = true;
	}
	ImGui::SameLine();

	if (ImGui::Button("EaseOut", ImVec2(PresetButtonWidth, 0)))
	{
		CameraShake->SetDecayCurve(FCubicBezierCurve::CreateEaseOut());
		bPresetSelected = true;
	}

	// 두 번째 줄
	if (ImGui::Button("EaseInOut", ImVec2(PresetButtonWidth, 0)))
	{
		CameraShake->SetDecayCurve(FCubicBezierCurve::CreateEaseInOut());
		bPresetSelected = true;
	}
	ImGui::SameLine();

	if (ImGui::Button("Bounce", ImVec2(PresetButtonWidth, 0)))
	{
		CameraShake->SetDecayCurve(FCubicBezierCurve::CreateBounce());
		bPresetSelected = true;
	}

	return bPresetSelected;
}

void FCameraShakeDetailPanel::DrawCurvePreview(UCameraModifier_CameraShake* CameraShake)
{
	ImGui::Text("Curve Preview");
	ImGui::Spacing();

	const FCubicBezierCurve& Curve = CameraShake->GetDecayCurve();

	// 캔버스 설정
	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize = ImVec2(250, PreviewGraphHeight);

	// 배경
	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	DrawList->AddRectFilled(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y), IM_COL32(40, 40, 40, 255));
	DrawList->AddRect(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y), IM_COL32(255, 255, 255, 255));

	// 그리드 (간단한 중앙선)
	DrawList->AddLine(
		ImVec2(CanvasPos.x, CanvasPos.y + CanvasSize.y * 0.5f),
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y * 0.5f),
		IM_COL32(100, 100, 100, 128)
	);

	// 곡선을 플롯으로 그리기
	const int32 Segments = 64;
	for (int32 i = 0; i < Segments; ++i)
	{
		float x0 = static_cast<float>(i) / static_cast<float>(Segments);
		float x1 = static_cast<float>(i + 1) / static_cast<float>(Segments);

		float y0 = Curve.SampleY(x0);
		float y1 = Curve.SampleY(x1);

		ImVec2 P0 = ImVec2(
			CanvasPos.x + x0 * CanvasSize.x,
			CanvasPos.y + (1.0f - y0) * CanvasSize.y
		);
		ImVec2 P1 = ImVec2(
			CanvasPos.x + x1 * CanvasSize.x,
			CanvasPos.y + (1.0f - y1) * CanvasSize.y
		);

		DrawList->AddLine(P0, P1, IM_COL32(100, 200, 255, 255), 2.0f);
	}

	// 현재 샘플 포인트 표시 (선택적)
	if (CameraShake->IsShaking())
	{
		// 흔들림 중이면 현재 진행 상황 표시
		// 이 부분은 실시간으로 업데이트하려면 매 프레임 호출되어야 함
		// 여기서는 간단히 중간점 표시
		float SampleX = 0.5f;
		float SampleY = Curve.SampleY(SampleX);

		ImVec2 SamplePos = ImVec2(
			CanvasPos.x + SampleX * CanvasSize.x,
			CanvasPos.y + (1.0f - SampleY) * CanvasSize.y
		);
		DrawList->AddCircleFilled(SamplePos, 4.0f, IM_COL32(255, 100, 100, 255));
	}

	ImGui::Dummy(CanvasSize);

	// 축 레이블
	ImGui::Text("X: Time [0->1]  |  Y: Amplitude Multiplier");
}

bool FCameraShakeDetailPanel::DrawShakeParameters(UCameraModifier_CameraShake* CameraShake)
{
	ImGui::Text("Shake Parameters");
	ImGui::Spacing();

	bool bParamChanged = false;

	// Duration
	TempDuration = 1.0f; // 기본값, 실제로는 CameraShake에서 가져와야 함
	if (ImGui::DragFloat("Duration (sec)", &TempDuration, 0.1f, 0.1f, 10.0f, "%.2f"))
	{
		bParamChanged = true;
	}

	// Location Amplitude
	TempLocationAmplitude = 5.0f;
	if (ImGui::DragFloat("Location Amplitude", &TempLocationAmplitude, 0.5f, 0.0f, 100.0f, "%.1f"))
	{
		bParamChanged = true;
	}

	// Rotation Amplitude
	TempRotationAmplitude = 2.0f;
	if (ImGui::DragFloat("Rotation Amplitude (deg)", &TempRotationAmplitude, 0.1f, 0.0f, 45.0f, "%.1f"))
	{
		bParamChanged = true;
	}

	// Pattern (Enum Combo)
	const char* PatternNames[] = { "Sine", "Perlin", "Random" };
	if (ImGui::Combo("Pattern", &TempPattern, PatternNames, 3))
	{
		bParamChanged = true;
	}

	// Frequency (Sine 패턴에만 유효)
	if (TempPattern == static_cast<int32>(ECameraShakePattern::Sine))
	{
		if (ImGui::DragFloat("Frequency (Hz)", &TempFrequency, 0.5f, 0.1f, 50.0f, "%.1f"))
		{
			bParamChanged = true;
		}
	}

	return bParamChanged;
}

void FCameraShakeDetailPanel::DrawTestControls(UCameraModifier_CameraShake* CameraShake, UWorld* World)
{
	ImGui::Text("Test Controls");
	ImGui::Spacing();

	bool bIsShaking = CameraShake->IsShaking();
	bool bIsPIEMode = World && (World->GetWorldType() == EWorldType::PIE);

	// Start 버튼 (PIE 모드가 아니거나 이미 흔들림 중이면 비활성화)
	ImGui::BeginDisabled(!bIsPIEMode || bIsShaking);
	if (ImGui::Button("Start Shake", ImVec2(120, 0)))
	{
		if (CameraShake->IsUsingDecayCurve())
		{
			// Bezier 곡선 기반 흔들림
			CameraShake->StartShakeWithCurve(
				TempDuration,
				TempLocationAmplitude,
				TempRotationAmplitude,
				static_cast<ECameraShakePattern>(TempPattern),
				TempFrequency,
				CameraShake->GetDecayCurve()
			);
		}
		else
		{
			// 기본 흔들림
			CameraShake->StartShake(
				TempDuration,
				TempLocationAmplitude,
				TempRotationAmplitude,
				static_cast<ECameraShakePattern>(TempPattern),
				TempFrequency
			);
		}
	}
	ImGui::EndDisabled();

	ImGui::SameLine();

	// Stop 버튼 (PIE 모드가 아니거나 흔들림 중이 아니면 비활성화)
	ImGui::BeginDisabled(!bIsPIEMode || !bIsShaking);
	if (ImGui::Button("Stop Shake", ImVec2(120, 0)))
	{
		CameraShake->StopShake();
	}
	ImGui::EndDisabled();

	// 현재 상태 표시
	ImGui::Spacing();
	if (bIsShaking)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: SHAKING");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Status: Idle");
	}
}
