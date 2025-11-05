#pragma once

#include "Global/Public/BezierCurve.h"
#include "imgui.h"

/**
 * @brief ImGui 기반 Cubic Bezier 곡선 에디터
 *
 * GitHub issue #786 (https://github.com/ocornut/imgui/issues/786)의
 * Bezier 곡선 에디터를 기반으로 구현되었습니다.
 *
 * 사용자가 4개의 제어점을 드래그하여 Bezier 곡선을 대화형으로 편집할 수 있습니다.
 * 그리드, 곡선 프리뷰, 제어점 핸들이 렌더링됩니다.
 *
 * 사용 예시:
 * ```cpp
 * FImGuiBezierEditor Editor;
 * FCubicBezierCurve Curve = FCubicBezierCurve::CreateEaseInOut();
 * if (Editor.Edit("MyCurve", Curve))
 * {
 *     // 곡선이 변경됨
 *     CameraShake->SetDecayCurve(Curve);
 * }
 * ```
 */
class FImGuiBezierEditor
{
public:
	FImGuiBezierEditor();
	~FImGuiBezierEditor();

	/**
	 * @brief Bezier 곡선 에디터 UI 렌더링
	 *
	 * @param Label 에디터 위젯의 고유 ID (ImGui ID로 사용)
	 * @param InOutCurve 편집할 곡선 (입출력)
	 * @param Size 에디터 캔버스 크기 (기본값: 200x200)
	 * @return true면 곡선이 변경됨, false면 변경 없음
	 */
	bool Edit(const char* Label, FCubicBezierCurve& InOutCurve, const ImVec2& Size = ImVec2(200, 200));

	/**
	 * @brief 특정 X 좌표에서 곡선 값을 미리보기 (선택적)
	 *
	 * @param X 입력 X 좌표 [0, 1]
	 * @param OutY 출력 Y 좌표 [0, 1]
	 * @return true면 유효한 값, false면 범위 외
	 */
	bool PreviewValue(float X, float& OutY, const FCubicBezierCurve& Curve) const;

private:
	/**
	 * @brief 마우스 입력 처리 및 제어점 드래그
	 *
	 * @param PointIndex 제어점 인덱스 [0, 3]
	 * @param InOutCurve 편집 중인 곡선
	 * @param CanvasPos 캔버스 좌상단 위치 (스크린 좌표)
	 * @param CanvasSize 캔버스 크기
	 * @return true면 포인트가 드래그됨
	 */
	bool HandlePointDrag(int32 PointIndex, FCubicBezierCurve& InOutCurve, const ImVec2& CanvasPos, const ImVec2& CanvasSize);

	/**
	 * @brief 캔버스에 그리드 렌더링
	 *
	 * @param DrawList ImGui 드로우 리스트
	 * @param CanvasPos 캔버스 좌상단 위치
	 * @param CanvasSize 캔버스 크기
	 */
	void DrawGrid(ImDrawList* DrawList, const ImVec2& CanvasPos, const ImVec2& CanvasSize);

	/**
	 * @brief Bezier 곡선 렌더링
	 *
	 * @param DrawList ImGui 드로우 리스트
	 * @param Curve 렌더링할 곡선
	 * @param CanvasPos 캔버스 좌상단 위치
	 * @param CanvasSize 캔버스 크기
	 */
	void DrawCurve(ImDrawList* DrawList, const FCubicBezierCurve& Curve, const ImVec2& CanvasPos, const ImVec2& CanvasSize);

	/**
	 * @brief 제어점 핸들 렌더링 (드래그 가능한 점)
	 *
	 * @param DrawList ImGui 드로우 리스트
	 * @param Curve 곡선
	 * @param CanvasPos 캔버스 좌상단 위치
	 * @param CanvasSize 캔버스 크기
	 * @param HoveredPoint 마우스가 올라간 포인트 인덱스 (-1이면 없음)
	 */
	void DrawControlPoints(ImDrawList* DrawList, const FCubicBezierCurve& Curve, const ImVec2& CanvasPos, const ImVec2& CanvasSize, int32 HoveredPoint);

	/**
	 * @brief 제어점 핸들 선 렌더링 (P0-P1, P2-P3 연결선)
	 *
	 * @param DrawList ImGui 드로우 리스트
	 * @param Curve 곡선
	 * @param CanvasPos 캔버스 좌상단 위치
	 * @param CanvasSize 캔버스 크기
	 */
	void DrawControlLines(ImDrawList* DrawList, const FCubicBezierCurve& Curve, const ImVec2& CanvasPos, const ImVec2& CanvasSize);

	/**
	 * @brief 정규화된 [0,1] 좌표를 캔버스 스크린 좌표로 변환
	 *
	 * @param Normalized 정규화된 좌표 [0, 1]
	 * @param CanvasPos 캔버스 좌상단 위치
	 * @param CanvasSize 캔버스 크기
	 * @return 스크린 좌표
	 */
	ImVec2 NormalizedToCanvas(const FVector2& Normalized, const ImVec2& CanvasPos, const ImVec2& CanvasSize) const;

	/**
	 * @brief 캔버스 스크린 좌표를 정규화된 [0,1] 좌표로 변환
	 *
	 * @param Canvas 캔버스 스크린 좌표
	 * @param CanvasPos 캔버스 좌상단 위치
	 * @param CanvasSize 캔버스 크기
	 * @return 정규화된 좌표 [0, 1]
	 */
	FVector2 CanvasToNormalized(const ImVec2& Canvas, const ImVec2& CanvasPos, const ImVec2& CanvasSize) const;

private:
	// 상태 변수
	int32 DraggedPoint;        // 현재 드래그 중인 포인트 인덱스 (-1이면 없음)
	bool bIsDragging;          // 드래그 상태 플래그

	// 스타일 설정
	float PointRadius;         // 제어점 반경 (픽셀)
	float CurveThickness;      // 곡선 두께 (픽셀)
	float GridAlpha;           // 그리드 투명도 [0, 1]
	int32 CurveSegments;       // 곡선을 그릴 때 세그먼트 수 (부드러움 제어)

	// 색상
	ImU32 GridColor;
	ImU32 CurveColor;
	ImU32 ControlLineColor;
	ImU32 PointColor;
	ImU32 PointHoverColor;
	ImU32 PointDragColor;
	ImU32 BackgroundColor;
};
