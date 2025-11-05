#include "pch.h"
#include "ImGuiBezierEditor.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>

FImGuiBezierEditor::FImGuiBezierEditor()
	: DraggedPoint(-1)
	, bIsDragging(false)
	, PointRadius(6.0f)
	, CurveThickness(2.0f)
	, GridAlpha(0.3f)
	, CurveSegments(64)
{
	// 색상 초기화 (ImGui 기본 색상 팔레트)
	GridColor = IM_COL32(100, 100, 100, static_cast<int>(255 * GridAlpha));
	CurveColor = IM_COL32(255, 200, 100, 255);        // 주황색
	ControlLineColor = IM_COL32(150, 150, 150, 200);  // 회색
	PointColor = IM_COL32(255, 255, 255, 255);        // 흰색
	PointHoverColor = IM_COL32(255, 255, 100, 255);   // 노란색
	PointDragColor = IM_COL32(255, 100, 100, 255);    // 빨간색
	BackgroundColor = IM_COL32(40, 40, 40, 255);      // 어두운 배경
}

FImGuiBezierEditor::~FImGuiBezierEditor()
{
}

bool FImGuiBezierEditor::Edit(const char* Label, FCubicBezierCurve& InOutCurve, const ImVec2& Size)
{
	bool bChanged = false;

	ImGui::PushID(Label);

	// 캔버스 영역 확보
	ImVec2 CanvasPos = ImGui::GetCursorScreenPos();
	ImVec2 CanvasSize = Size;

	// 캔버스를 클릭 가능한 아이템으로 만들기
	ImGui::InvisibleButton("canvas", CanvasSize);
	bool bIsHovered = ImGui::IsItemHovered();
	bool bIsActive = ImGui::IsItemActive();

	// 드로우 리스트 가져오기
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	// 배경 렌더링
	DrawList->AddRectFilled(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y), BackgroundColor);
	DrawList->AddRect(CanvasPos, ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y), IM_COL32(255, 255, 255, 255));

	// 그리드 렌더링
	DrawGrid(DrawList, CanvasPos, CanvasSize);

	// 제어선 렌더링 (곡선 아래)
	DrawControlLines(DrawList, InOutCurve, CanvasPos, CanvasSize);

	// 곡선 렌더링
	DrawCurve(DrawList, InOutCurve, CanvasPos, CanvasSize);

	// 마우스 상호작용 처리
	int32 HoveredPoint = -1;

	if (bIsHovered || bIsDragging)
	{
		ImVec2 MousePos = ImGui::GetMousePos();

		// 드래그 중이 아니면 호버된 포인트 찾기
		if (!bIsDragging)
		{
			for (int32 i = 0; i < 4; ++i)
			{
				ImVec2 PointPos = NormalizedToCanvas(InOutCurve.P[i], CanvasPos, CanvasSize);
				float Distance = std::sqrt((MousePos.x - PointPos.x) * (MousePos.x - PointPos.x) +
				                          (MousePos.y - PointPos.y) * (MousePos.y - PointPos.y));

				if (Distance < PointRadius + 2.0f)
				{
					HoveredPoint = i;
					break;
				}
			}

			// 마우스 클릭 시작 시 드래그 시작
			if (HoveredPoint >= 0 && ImGui::IsMouseClicked(0))
			{
				DraggedPoint = HoveredPoint;
				bIsDragging = true;
			}
		}
		else
		{
			// 드래그 중
			HoveredPoint = DraggedPoint;

			if (HandlePointDrag(DraggedPoint, InOutCurve, CanvasPos, CanvasSize))
			{
				bChanged = true;
			}

			// 마우스 릴리즈 시 드래그 종료
			if (ImGui::IsMouseReleased(0))
			{
				bIsDragging = false;
				DraggedPoint = -1;
			}
		}
	}

	// 제어점 렌더링
	DrawControlPoints(DrawList, InOutCurve, CanvasPos, CanvasSize, HoveredPoint);

	ImGui::PopID();

	return bChanged;
}

bool FImGuiBezierEditor::PreviewValue(float X, float& OutY, const FCubicBezierCurve& Curve) const
{
	if (X < 0.0f || X > 1.0f)
		return false;

	OutY = Curve.SampleY(X);
	return true;
}

bool FImGuiBezierEditor::HandlePointDrag(int32 PointIndex, FCubicBezierCurve& InOutCurve, const ImVec2& CanvasPos, const ImVec2& CanvasSize)
{
	if (PointIndex < 0 || PointIndex >= 4)
		return false;

	ImVec2 MousePos = ImGui::GetMousePos();
	FVector2 NewPos = CanvasToNormalized(MousePos, CanvasPos, CanvasSize);

	// P0와 P3는 X 좌표 고정 (각각 0과 1)
	if (PointIndex == 0)
	{
		NewPos.X = 0.0f;
	}
	else if (PointIndex == 3)
	{
		NewPos.X = 1.0f;
	}

	// Y 좌표 제한 (선택적으로 [0, 1] 범위를 벗어나도 허용 가능)
	// 여기서는 자유롭게 허용 (Bounce 효과 등을 위해)
	// 필요하면 Clamp 추가 가능:
	// NewPos.Y = Clamp(NewPos.Y, 0.0f, 1.0f);

	bool bChanged = (InOutCurve.P[PointIndex].X != NewPos.X || InOutCurve.P[PointIndex].Y != NewPos.Y);

	if (bChanged)
	{
		InOutCurve.P[PointIndex] = NewPos;
	}

	return bChanged;
}

void FImGuiBezierEditor::DrawGrid(ImDrawList* DrawList, const ImVec2& CanvasPos, const ImVec2& CanvasSize)
{
	const int32 GridLines = 4; // 4x4 그리드

	for (int32 i = 0; i <= GridLines; ++i)
	{
		float t = static_cast<float>(i) / static_cast<float>(GridLines);

		// 수직선
		float x = CanvasPos.x + t * CanvasSize.x;
		DrawList->AddLine(ImVec2(x, CanvasPos.y), ImVec2(x, CanvasPos.y + CanvasSize.y), GridColor);

		// 수평선
		float y = CanvasPos.y + t * CanvasSize.y;
		DrawList->AddLine(ImVec2(CanvasPos.x, y), ImVec2(CanvasPos.x + CanvasSize.x, y), GridColor);
	}
}

void FImGuiBezierEditor::DrawCurve(ImDrawList* DrawList, const FCubicBezierCurve& Curve, const ImVec2& CanvasPos, const ImVec2& CanvasSize)
{
	// Bezier 곡선을 여러 세그먼트로 나누어 그리기
	for (int32 i = 0; i < CurveSegments; ++i)
	{
		float t0 = static_cast<float>(i) / static_cast<float>(CurveSegments);
		float t1 = static_cast<float>(i + 1) / static_cast<float>(CurveSegments);

		FVector2 P0 = Curve.Evaluate(t0);
		FVector2 P1 = Curve.Evaluate(t1);

		ImVec2 ScreenP0 = NormalizedToCanvas(P0, CanvasPos, CanvasSize);
		ImVec2 ScreenP1 = NormalizedToCanvas(P1, CanvasPos, CanvasSize);

		DrawList->AddLine(ScreenP0, ScreenP1, CurveColor, CurveThickness);
	}
}

void FImGuiBezierEditor::DrawControlPoints(ImDrawList* DrawList, const FCubicBezierCurve& Curve, const ImVec2& CanvasPos, const ImVec2& CanvasSize, int32 HoveredPoint)
{
	for (int32 i = 0; i < 4; ++i)
	{
		ImVec2 PointPos = NormalizedToCanvas(Curve.P[i], CanvasPos, CanvasSize);

		// 포인트 색상 결정
		ImU32 Color = PointColor;
		if (i == HoveredPoint)
		{
			if (bIsDragging)
				Color = PointDragColor;
			else
				Color = PointHoverColor;
		}

		// 외곽선
		DrawList->AddCircleFilled(PointPos, PointRadius, Color);
		DrawList->AddCircle(PointPos, PointRadius, IM_COL32(0, 0, 0, 255), 12, 1.5f);

		// 포인트 번호 표시 (선택적)
		char LabelText[8];
		snprintf(LabelText, sizeof(LabelText), "P%d", i);
		ImVec2 TextPos = ImVec2(PointPos.x + PointRadius + 4.0f, PointPos.y - 8.0f);
		DrawList->AddText(TextPos, IM_COL32(255, 255, 255, 200), LabelText);
	}
}

void FImGuiBezierEditor::DrawControlLines(ImDrawList* DrawList, const FCubicBezierCurve& Curve, const ImVec2& CanvasPos, const ImVec2& CanvasSize)
{
	// P0 -> P1 연결선
	ImVec2 P0 = NormalizedToCanvas(Curve.P[0], CanvasPos, CanvasSize);
	ImVec2 P1 = NormalizedToCanvas(Curve.P[1], CanvasPos, CanvasSize);
	DrawList->AddLine(P0, P1, ControlLineColor, 1.0f);

	// P2 -> P3 연결선
	ImVec2 P2 = NormalizedToCanvas(Curve.P[2], CanvasPos, CanvasSize);
	ImVec2 P3 = NormalizedToCanvas(Curve.P[3], CanvasPos, CanvasSize);
	DrawList->AddLine(P2, P3, ControlLineColor, 1.0f);
}

ImVec2 FImGuiBezierEditor::NormalizedToCanvas(const FVector2& Normalized, const ImVec2& CanvasPos, const ImVec2& CanvasSize) const
{
	// 정규화된 좌표 [0, 1] -> 캔버스 스크린 좌표
	// Y축 반전: ImGui는 Y축이 아래로 증가, Bezier는 위로 증가
	return ImVec2(
		CanvasPos.x + Normalized.X * CanvasSize.x,
		CanvasPos.y + (1.0f - Normalized.Y) * CanvasSize.y  // Y 반전
	);
}

FVector2 FImGuiBezierEditor::CanvasToNormalized(const ImVec2& Canvas, const ImVec2& CanvasPos, const ImVec2& CanvasSize) const
{
	// 캔버스 스크린 좌표 -> 정규화된 좌표 [0, 1]
	// Y축 반전
	FVector2 Result;
	Result.X = (Canvas.x - CanvasPos.x) / CanvasSize.x;
	Result.Y = 1.0f - ((Canvas.y - CanvasPos.y) / CanvasSize.y);  // Y 반전

	return Result;
}
