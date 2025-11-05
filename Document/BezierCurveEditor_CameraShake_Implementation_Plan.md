# Cubic Bezier Curve Editor - Camera Shake UI 구현 계획

**작성일**: 2025-01-05
**목표**: ImGui Cubic Bezier curve editor를 사용하여 Camera Shake 감쇠 패턴을 시각적으로 디자인하고 제어하는 Detail 패널 구현
**참고**: GitHub Issue #786 (https://github.com/ocornut/imgui/issues/786)

---

## 전체 작업 개요

| Phase | 작업 내용 | 예상 시간 | 상태 |
|-------|----------|----------|------|
| **Phase 1** | Bezier Curve 시스템 구축 | 1.5시간 | ⬜ Pending |
| **Phase 2** | ImGui Editor 구현 | 2시간 | ⬜ Pending |
| **Phase 3** | Detail 패널 UI | 2.5시간 | ⬜ Pending |
| **Phase 4** | 빌드 시스템 | 45분 | ⬜ Pending |
| **Phase 5** | 테스트 | 1.5시간 | ⬜ Pending |
| **Phase 6** | 문서화 & 최적화 | 1시간 | ⬜ Pending |
| **총합** | | **~9-10시간** | |

---

## Phase 1: Bezier Curve 기반 시스템 구축

### 1.1 Bezier Curve 데이터 구조 생성 ⬜

**목표**: Cubic Bezier curve를 표현하고 평가하는 기본 구조 구축

#### 파일 생성
- `Engine/Source/Global/Public/BezierCurve.h` (신규)
- `Engine/Source/Global/Private/BezierCurve.cpp` (신규)

#### 구현 내용

```cpp
// BezierCurve.h
#pragma once

/**
 * @brief Cubic Bezier Curve - 4개의 제어점으로 정의되는 커브
 * @note 애니메이션, 카메라 셰이크 감쇠 등에 사용
 */
struct FCubicBezierCurve
{
    float P[4]; // 4개의 제어점 [P0, P1, P2, P3]

    FCubicBezierCurve();
    FCubicBezierCurve(float P0, float P1, float P2, float P3);

    /**
     * @brief Bezier curve에서 특정 시간(0~1)의 값을 평가
     * @param t 시간 [0.0, 1.0]
     * @return 커브 값
     */
    float Evaluate(float t) const;

    /**
     * @brief JSON 직렬화
     */
    void Serialize(bool bInIsLoading, JSON& InOutHandle);

    // 프리셋 정적 함수들
    static FCubicBezierCurve Linear();
    static FCubicBezierCurve EaseInOut();
    static FCubicBezierCurve EaseIn();
    static FCubicBezierCurve EaseOut();
    static FCubicBezierCurve Bounce();
};
```

```cpp
// BezierCurve.cpp
#include "pch.h"
#include "Global/Public/BezierCurve.h"

FCubicBezierCurve::FCubicBezierCurve()
    : P{0.0f, 0.33f, 0.66f, 1.0f} // 기본값: Linear
{}

FCubicBezierCurve::FCubicBezierCurve(float P0, float P1, float P2, float P3)
    : P{P0, P1, P2, P3}
{}

float FCubicBezierCurve::Evaluate(float t) const
{
    // Clamp t to [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Cubic Bezier formula: B(t) = (1-t)³P0 + 3(1-t)²t·P1 + 3(1-t)t²·P2 + t³·P3
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * P[0] + 3.0f * uu * t * P[1] + 3.0f * u * tt * P[2] + ttt * P[3];
}

void FCubicBezierCurve::Serialize(bool bInIsLoading, JSON& InOutHandle)
{
    if (bInIsLoading)
    {
        if (InOutHandle.hasKey("P0")) P[0] = static_cast<float>(InOutHandle["P0"].ToFloat());
        if (InOutHandle.hasKey("P1")) P[1] = static_cast<float>(InOutHandle["P1"].ToFloat());
        if (InOutHandle.hasKey("P2")) P[2] = static_cast<float>(InOutHandle["P2"].ToFloat());
        if (InOutHandle.hasKey("P3")) P[3] = static_cast<float>(InOutHandle["P3"].ToFloat());
    }
    else
    {
        InOutHandle["P0"] = P[0];
        InOutHandle["P1"] = P[1];
        InOutHandle["P2"] = P[2];
        InOutHandle["P3"] = P[3];
    }
}

// 프리셋들
FCubicBezierCurve FCubicBezierCurve::Linear()
{
    return FCubicBezierCurve(0.0f, 0.33f, 0.66f, 1.0f);
}

FCubicBezierCurve FCubicBezierCurve::EaseInOut()
{
    return FCubicBezierCurve(0.0f, 0.0f, 1.0f, 1.0f);
}

FCubicBezierCurve FCubicBezierCurve::EaseIn()
{
    return FCubicBezierCurve(0.42f, 0.0f, 1.0f, 1.0f);
}

FCubicBezierCurve FCubicBezierCurve::EaseOut()
{
    return FCubicBezierCurve(0.0f, 0.0f, 0.58f, 1.0f);
}

FCubicBezierCurve FCubicBezierCurve::Bounce()
{
    return FCubicBezierCurve(0.68f, -0.55f, 0.265f, 1.55f);
}
```

#### 체크리스트
- [ ] BezierCurve.h 파일 생성
- [ ] BezierCurve.cpp 파일 생성
- [ ] Evaluate() 함수 구현 (Cubic Bezier formula)
- [ ] Serialize() 함수 구현 (JSON)
- [ ] 프리셋 5개 구현 (Linear, EaseIn, EaseOut, EaseInOut, Bounce)
- [ ] 빌드 테스트 (단독 컴파일 확인)

---

### 1.2 UCameraModifier_CameraShake에 Bezier Curve 통합 ⬜

**목표**: Camera Shake가 Bezier curve를 사용하여 감쇠를 제어하도록 수정

#### 파일 수정
- `Engine/Source/Component/Camera/Public/CameraModifier_CameraShake.h`
- `Engine/Source/Component/Camera/Private/CameraModifier_CameraShake.cpp`

#### 수정 내용

**CameraModifier_CameraShake.h**:
```cpp
#pragma once
#include "Component/Camera/Public/CameraModifier.h"
#include "Global/Public/BezierCurve.h"  // 추가

// ... 기존 코드 ...

class UCameraModifier_CameraShake : public UCameraModifier
{
    // ... 기존 코드 ...

private:
    // ===== Bezier Curve 추가 =====
    FCubicBezierCurve DecayCurve;  // 감쇠 커브

public:
    // ===== Getter/Setter 추가 =====
    float GetShakeDuration() const { return ShakeDuration; }
    float GetShakeTimeRemaining() const { return ShakeTimeRemaining; }
    float GetLocationAmplitude() const { return LocationAmplitude; }
    float GetRotationAmplitude() const { return RotationAmplitude; }
    ECameraShakePattern GetPattern() const { return Pattern; }
    float GetFrequency() const { return Frequency; }

    void SetLocationAmplitude(float InAmplitude) { LocationAmplitude = InAmplitude; }
    void SetRotationAmplitude(float InAmplitude) { RotationAmplitude = InAmplitude; }
    void SetFrequency(float InFrequency) { Frequency = InFrequency; }
    void SetPattern(ECameraShakePattern InPattern) { Pattern = InPattern; }

    const FCubicBezierCurve& GetDecayCurve() const { return DecayCurve; }
    void SetDecayCurve(const FCubicBezierCurve& InCurve) { DecayCurve = InCurve; }

    /**
     * @brief 커스텀 감쇠 커브로 흔들림 시작
     */
    void StartShakeWithCurve(
        float InDuration = 1.0f,
        float InLocationAmplitude = 5.0f,
        float InRotationAmplitude = 2.0f,
        ECameraShakePattern InPattern = ECameraShakePattern::Perlin,
        float InFrequency = 10.0f,
        const FCubicBezierCurve& InDecayCurve = FCubicBezierCurve::Linear()
    );

    /**
     * @brief 파형 샘플링 (그래프 시각화용)
     * @param Time 샘플링할 시간
     * @param OutLocationOffset 출력 위치 오프셋
     * @param OutRotationOffset 출력 회전 오프셋
     */
    void SampleShakeAt(float Time, FVector& OutLocationOffset, FVector& OutRotationOffset) const;
};
```

**CameraModifier_CameraShake.cpp**:
```cpp
void UCameraModifier_CameraShake::StartShakeWithCurve(
    float InDuration,
    float InLocationAmplitude,
    float InRotationAmplitude,
    ECameraShakePattern InPattern,
    float InFrequency,
    const FCubicBezierCurve& InDecayCurve)
{
    // 기존 StartShake 호출
    StartShake(InDuration, InLocationAmplitude, InRotationAmplitude, InPattern, InFrequency);

    // 커스텀 커브 설정
    DecayCurve = InDecayCurve;

    UE_LOG_DEBUG("UCameraModifier_CameraShake: Bezier curve로 흔들림 시작 (Duration=%.2f)", InDuration);
}

bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    if (!bIsShaking)
        return false;

    ShakeTimeRemaining -= DeltaTime;
    ShakeTime += DeltaTime;

    if (ShakeTimeRemaining <= 0.0f)
    {
        StopShake();
        return false;
    }

    // ===== Bezier curve 기반 감쇠 계산 =====
    float NormalizedTime = 1.0f - (ShakeTimeRemaining / ShakeDuration); // 0~1
    float DecayAlpha = 1.0f - DecayCurve.Evaluate(NormalizedTime);      // Bezier curve 평가

    // 흔들림 오프셋 평가
    FVector LocationOffset, RotationOffset;
    EvaluateShake(LocationOffset, RotationOffset, ShakeTime, DecayAlpha);

    // POV에 적용
    InOutPOV.Location = InOutPOV.Location + LocationOffset;

    FQuaternion RotationQuat = FQuaternion::FromEulerAngles(RotationOffset);
    InOutPOV.Rotation = InOutPOV.Rotation * RotationQuat;

    return true;
}

void UCameraModifier_CameraShake::SampleShakeAt(float Time, FVector& OutLocationOffset, FVector& OutRotationOffset) const
{
    // 감쇠 없이 샘플링 (미리보기용)
    float DecayAlpha = 1.0f;

    OutLocationOffset = FVector(0, 0, 0);
    OutRotationOffset = FVector(0, 0, 0);

    switch (Pattern)
    {
    case ECameraShakePattern::Sine:
        {
            float Omega = 2.0f * PI * Frequency;
            OutLocationOffset.X = LocationAmplitude * std::sin(Omega * Time);
            OutLocationOffset.Y = LocationAmplitude * std::cos(Omega * Time * 1.3f);
            OutLocationOffset.Z = LocationAmplitude * std::sin(Omega * Time * 0.7f);

            OutRotationOffset.X = RotationAmplitude * std::sin(Omega * Time * 1.1f);
            OutRotationOffset.Y = RotationAmplitude * std::cos(Omega * Time * 0.9f);
            OutRotationOffset.Z = RotationAmplitude * std::sin(Omega * Time * 1.2f);
        }
        break;

    case ECameraShakePattern::Perlin:
        {
            OutLocationOffset.X = LocationAmplitude * PerlinNoise1D(Time * 10.0f);
            OutLocationOffset.Y = LocationAmplitude * PerlinNoise1D(Time * 10.0f + 100.0f);
            OutLocationOffset.Z = LocationAmplitude * PerlinNoise1D(Time * 10.0f + 200.0f);

            OutRotationOffset.X = RotationAmplitude * PerlinNoise1D(Time * 10.0f + 300.0f);
            OutRotationOffset.Y = RotationAmplitude * PerlinNoise1D(Time * 10.0f + 400.0f);
            OutRotationOffset.Z = RotationAmplitude * PerlinNoise1D(Time * 10.0f + 500.0f);
        }
        break;

    case ECameraShakePattern::Random:
        // Random은 샘플링 의미 없음
        break;
    }
}
```

#### 체크리스트
- [ ] CameraModifier_CameraShake.h에 `#include "Global/Public/BezierCurve.h"` 추가
- [ ] DecayCurve 멤버 변수 추가
- [ ] Getter/Setter 10개 추가
- [ ] StartShakeWithCurve() 구현
- [ ] ModifyCamera()에서 DecayCurve.Evaluate() 사용하도록 수정
- [ ] SampleShakeAt() 구현
- [ ] 빌드 테스트

---

## Phase 2: ImGui Bezier Curve Editor 구현

### 2.1 Bezier Curve Editor 위젯 구현 ⬜

**목표**: GitHub 이슈 #786 기반의 인터랙티브 Bezier editor 구현

#### 파일 생성
- `Engine/Source/Render/UI/Public/ImGuiBezierEditor.h` (신규)
- `Engine/Source/Render/UI/Private/ImGuiBezierEditor.cpp` (신규)

#### 구현 내용

**ImGuiBezierEditor.h**:
```cpp
#pragma once
#include <imgui.h>

/**
 * @brief ImGui Bezier Curve Editor 위젯
 * @note GitHub issue #786 기반 구현
 */
class FImGuiBezierEditor
{
public:
    /**
     * @brief Bezier curve 에디터 위젯 렌더링
     * @param label 위젯 ID
     * @param P Bezier 제어점 배열 [4]
     * @param size 에디터 크기 (기본: 128x128)
     * @return 값이 변경되었으면 true
     */
    static bool BezierCurve(const char* label, float P[4], const ImVec2& size = ImVec2(128, 128));

private:
    /**
     * @brief Bezier curve 포인트 계산 (Bernstein polynomial)
     */
    static void BezierTable(float P[4], ImVec2* results, int32 count);

    /**
     * @brief X 위치에서 Y 값 찾기
     */
    static float BezierValue(float t, float P[4]);
};
```

**ImGuiBezierEditor.cpp**:
```cpp
#include "pch.h"
#include "Render/UI/Public/ImGuiBezierEditor.h"
#include <cmath>

void FImGuiBezierEditor::BezierTable(float P[4], ImVec2* results, int32 count)
{
    for (int32 step = 0; step < count; ++step)
    {
        float t = static_cast<float>(step) / (count - 1);
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        results[step].x = uuu * P[0] + 3.0f * uu * t * P[1] + 3.0f * u * tt * P[2] + ttt * P[3];
        results[step].y = results[step].x;
    }
}

float FImGuiBezierEditor::BezierValue(float t, float P[4])
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    return uuu * P[0] + 3.0f * uu * t * P[1] + 3.0f * u * tt * P[2] + ttt * P[3];
}

bool FImGuiBezierEditor::BezierCurve(const char* label, float P[4], const ImVec2& size)
{
    constexpr int32 SMOOTHNESS = 64;
    constexpr float CURVE_WIDTH = 4.0f;
    constexpr float GRAB_RADIUS = 6.0f;
    constexpr float GRAB_BORDER = 2.0f;

    bool changed = false;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushID(label);

    // Canvas 영역
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = size;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 배경 (어두운 회색)
    draw_list->AddRectFilled(canvas_pos,
                             ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                             IM_COL32(50, 50, 50, 255));

    // 격자 그리기 (4x4)
    for (int32 i = 0; i <= 4; ++i)
    {
        float x = canvas_pos.x + (canvas_size.x / 4.0f) * i;
        float y = canvas_pos.y + (canvas_size.y / 4.0f) * i;
        draw_list->AddLine(ImVec2(x, canvas_pos.y),
                          ImVec2(x, canvas_pos.y + canvas_size.y),
                          IM_COL32(70, 70, 70, 255));
        draw_list->AddLine(ImVec2(canvas_pos.x, y),
                          ImVec2(canvas_pos.x + canvas_size.x, y),
                          IM_COL32(70, 70, 70, 255));
    }

    // Bezier curve 계산
    ImVec2 curve_points[SMOOTHNESS];
    BezierTable(P, curve_points, SMOOTHNESS);

    // Curve 그리기 (노란색)
    for (int32 i = 0; i < SMOOTHNESS - 1; ++i)
    {
        ImVec2 p1 = ImVec2(
            canvas_pos.x + curve_points[i].x * canvas_size.x,
            canvas_pos.y + canvas_size.y - curve_points[i].y * canvas_size.y
        );
        ImVec2 p2 = ImVec2(
            canvas_pos.x + curve_points[i + 1].x * canvas_size.x,
            canvas_pos.y + canvas_size.y - curve_points[i + 1].y * canvas_size.y
        );
        draw_list->AddLine(p1, p2, IM_COL32(255, 255, 0, 255), CURVE_WIDTH);
    }

    // 제어점 P1, P2 그리기 및 드래그 처리
    ImVec2 control_points[2] = {
        ImVec2(canvas_pos.x + P[1] * canvas_size.x,
               canvas_pos.y + canvas_size.y - P[1] * canvas_size.y),
        ImVec2(canvas_pos.x + P[2] * canvas_size.x,
               canvas_pos.y + canvas_size.y - P[2] * canvas_size.y)
    };

    ImU32 colors[2] = { IM_COL32(255, 100, 255, 255), IM_COL32(100, 255, 255, 255) }; // 핑크, 시안

    // 드래그 처리
    for (int32 i = 0; i < 2; ++i)
    {
        ImVec2& cp = control_points[i];

        // 제어점 원 그리기
        draw_list->AddCircleFilled(cp, GRAB_RADIUS, colors[i]);
        draw_list->AddCircle(cp, GRAB_RADIUS, IM_COL32(255, 255, 255, 255), 16, GRAB_BORDER);

        // 드래그 감지
        ImRect grab_rect(ImVec2(cp.x - GRAB_RADIUS, cp.y - GRAB_RADIUS),
                        ImVec2(cp.x + GRAB_RADIUS, cp.y + GRAB_RADIUS));

        if (ImGui::IsMouseHoveringRect(grab_rect.Min, grab_rect.Max))
        {
            ImGui::SetTooltip("%.3f, %.3f", P[i + 1], P[i + 1]);

            if (ImGui::IsMouseDown(0))
            {
                ImVec2 mouse_pos = io.MousePos;
                float new_val = (mouse_pos.x - canvas_pos.x) / canvas_size.x;
                new_val = ImClamp(new_val, 0.0f, 1.0f);

                if (P[i + 1] != new_val)
                {
                    P[i + 1] = new_val;
                    changed = true;
                }
            }
        }
    }

    // Invisible button for interaction
    ImGui::InvisibleButton("canvas", canvas_size);

    // Slider (아래쪽)
    if (ImGui::SliderFloat("##P1", &P[1], 0.0f, 1.0f, "P1: %.3f"))
        changed = true;
    if (ImGui::SliderFloat("##P2", &P[2], 0.0f, 1.0f, "P2: %.3f"))
        changed = true;

    ImGui::PopID();
    return changed;
}
```

#### 체크리스트
- [ ] ImGuiBezierEditor.h 파일 생성
- [ ] ImGuiBezierEditor.cpp 파일 생성
- [ ] BezierTable() 구현 (Bernstein polynomial)
- [ ] BezierValue() 구현
- [ ] BezierCurve() 메인 위젯 구현:
  - [ ] 128x128 캔버스 생성
  - [ ] 격자 배경 (4x4)
  - [ ] Bezier curve 렌더링 (노란색)
  - [ ] 제어점 P1, P2 원 그리기 (핑크, 시안)
  - [ ] 드래그 가능하게 만들기
  - [ ] 툴팁 표시
  - [ ] 슬라이더 2개 추가
- [ ] 빌드 테스트

---

## Phase 3: Detail 패널 UI 구현

### 3.1 Camera Shake Detail 패널 생성 ⬜

**목표**: ImGui 윈도우에 Camera Shake 제어 UI 구현

#### 파일 생성
- `Engine/Source/Render/UI/Public/CameraShakeDetailPanel.h` (신규)
- `Engine/Source/Render/UI/Private/CameraShakeDetailPanel.cpp` (신규)

#### 구현 내용

**CameraShakeDetailPanel.h**:
```cpp
#pragma once

class UCameraModifier_CameraShake;

/**
 * @brief Camera Shake Detail 패널 - Bezier curve 기반 제어 UI
 */
class FCameraShakeDetailPanel
{
public:
    /**
     * @brief Detail 패널 렌더링
     * @param ShakeMod Camera Shake 모디파이어 (nullptr 가능)
     */
    static void Draw(UCameraModifier_CameraShake* ShakeMod);

private:
    // UI 상태 (static)
    static float StaticBezierP[4];
    static float StaticDuration;
    static float StaticLocationAmp;
    static float StaticRotationAmp;
    static int32 StaticPattern;
    static float StaticFrequency;
};
```

**CameraShakeDetailPanel.cpp**: (상세 구현은 파일 내 주석 참고)

주요 섹션:
1. 상태 표시 (IsShaking, TimeRemaining)
2. 파라미터 조정 슬라이더 (Duration, Amplitude, Frequency, Pattern)
3. Bezier Curve Editor (256x256)
4. 프리셋 버튼 (Linear, Ease In, Ease Out, Ease In Out, Bounce)
5. 미리보기 그래프 (Location X/Y/Z, Rotation X/Y/Z)
6. 제어 버튼 (Start/Stop Shake)

#### 체크리스트
- [ ] CameraShakeDetailPanel.h 생성
- [ ] CameraShakeDetailPanel.cpp 생성
- [ ] static 변수 초기화
- [ ] 섹션 1: 상태 표시 구현
- [ ] 섹션 2: 파라미터 슬라이더 구현
- [ ] 섹션 3: Bezier editor 통합
- [ ] 섹션 4: 프리셋 버튼 5개 구현
- [ ] 섹션 5: 미리보기 그래프 구현
- [ ] 섹션 6: Start/Stop 버튼 구현
- [ ] 빌드 테스트

---

### 3.2 Editor 윈도우에 통합 ⬜

**목표**: Editor UI에서 Detail 패널 접근 가능하게 만들기

#### 파일 수정
- `Engine/Source/Editor/Private/EditorEngine.cpp` 또는
- `Engine/Source/Render/UI/Private/UIManager.cpp`

#### 수정 내용
```cpp
// EditorEngine.cpp 또는 UIManager.cpp
void DrawEditorWindows()
{
    // Menu bar에 항목 추가
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Windows"))
        {
            if (ImGui::MenuItem("Camera Shake Detail"))
            {
                bShowCameraShakeDetail = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (bShowCameraShakeDetail)
    {
        APlayerCameraManager* CameraManager = GetCurrentCameraManager();
        if (CameraManager)
        {
            UCameraModifier_CameraShake* ShakeMod =
                Cast<UCameraModifier_CameraShake>(
                    CameraManager->FindCameraModifierByClass(
                        UCameraModifier_CameraShake::StaticClass()
                    )
                );

            if (!ShakeMod)
            {
                ImGui::Begin("Camera Shake Detail");
                ImGui::Text("No CameraShake modifier found!");
                if (ImGui::Button("Add CameraShake Modifier"))
                {
                    CameraManager->AddCameraModifier(
                        UCameraModifier_CameraShake::StaticClass()
                    );
                }
                ImGui::End();
            }
            else
            {
                FCameraShakeDetailPanel::Draw(ShakeMod);
            }
        }
    }
}
```

#### 체크리스트
- [ ] bShowCameraShakeDetail 플래그 추가
- [ ] Menu bar 항목 추가
- [ ] CameraManager 찾기 로직 구현
- [ ] "Add Modifier" 버튼 구현
- [ ] FCameraShakeDetailPanel::Draw() 호출
- [ ] 빌드 테스트

---

## Phase 4: 빌드 시스템 설정

### 4.1 프로젝트 파일 업데이트 ⬜

**목표**: 새 파일들을 프로젝트에 추가

#### 파일 수정
- `Engine/Engine.vcxproj`
- `Engine/Engine.vcxproj.filters`

#### 추가할 파일 목록
```xml
<!-- ClCompile -->
<ClCompile Include="Source\Global\Private\BezierCurve.cpp" />
<ClCompile Include="Source\Render\UI\Private\ImGuiBezierEditor.cpp" />
<ClCompile Include="Source\Render\UI\Private\CameraShakeDetailPanel.cpp" />

<!-- ClInclude -->
<ClInclude Include="Source\Global\Public\BezierCurve.h" />
<ClInclude Include="Source\Render\UI\Public\ImGuiBezierEditor.h" />
<ClInclude Include="Source\Render\UI\Public\CameraShakeDetailPanel.h" />
```

#### 체크리스트
- [ ] Engine.vcxproj 열기
- [ ] ClCompile 섹션에 .cpp 파일 3개 추가
- [ ] ClInclude 섹션에 .h 파일 3개 추가
- [ ] Engine.vcxproj.filters 업데이트 (폴더 구조)
- [ ] Visual Studio에서 프로젝트 다시 로드

---

### 4.2 빌드 및 컴파일 오류 수정 ⬜

**목표**: 모든 파일이 정상적으로 컴파일되도록 수정

#### 체크리스트
- [ ] 전체 빌드 (Debug x64)
- [ ] Include 경로 오류 수정
- [ ] 전방 선언 추가 (필요 시)
- [ ] 링커 오류 해결
- [ ] 경고 제거 (C4244 등)
- [ ] 빌드 성공 확인 (0 errors)

---

## Phase 5: 테스트 및 검증

### 5.1 기능 테스트 ⬜

**목표**: 모든 기능이 정상 동작하는지 검증

#### 테스트 시나리오

| # | 테스트 | 예상 결과 | 상태 |
|---|--------|----------|------|
| 1 | Editor 실행 → Windows → Camera Shake Detail 열기 | 윈도우 열림 | ⬜ |
| 2 | CameraShake Modifier 없으면 "Add" 버튼 클릭 | Modifier 추가됨 | ⬜ |
| 3 | Duration 슬라이더 조정 | ShakeMod 값 실시간 변경 | ⬜ |
| 4 | Location Amplitude 슬라이더 조정 | ShakeMod 값 실시간 변경 | ⬜ |
| 5 | Rotation Amplitude 슬라이더 조정 | ShakeMod 값 실시간 변경 | ⬜ |
| 6 | Frequency 슬라이더 조정 | ShakeMod 값 실시간 변경 | ⬜ |
| 7 | Pattern Combo 변경 (Sine/Perlin/Random) | 패턴 변경 확인 | ⬜ |
| 8 | Bezier curve editor에서 P1 드래그 | 커브 모양 변경, 미리보기 업데이트 | ⬜ |
| 9 | Bezier curve editor에서 P2 드래그 | 커브 모양 변경, 미리보기 업데이트 | ⬜ |
| 10 | "Linear" 프리셋 버튼 클릭 | 커브 즉시 Linear로 변경 | ⬜ |
| 11 | "Ease In" 프리셋 버튼 클릭 | 커브 즉시 Ease In으로 변경 | ⬜ |
| 12 | "Ease Out" 프리셋 버튼 클릭 | 커브 즉시 Ease Out으로 변경 | ⬜ |
| 13 | "Bounce" 프리셋 버튼 클릭 | 커브 Bounce로 변경 (음수 가능) | ⬜ |
| 14 | "Start Shake" 버튼 클릭 | 카메라 흔들림 시작 | ⬜ |
| 15 | 미리보기 그래프와 실제 흔들림 비교 | 일치 확인 | ⬜ |
| 16 | "Stop Shake" 버튼 클릭 | 흔들림 즉시 중지 | ⬜ |

#### 체크리스트
- [ ] 모든 테스트 시나리오 실행
- [ ] 버그 발견 시 수정
- [ ] 재테스트
- [ ] 테스트 결과 문서화

---

### 5.2 ACameraTestActor 통합 테스트 ⬜

**목표**: CameraTestActor에서 Bezier curve 자동 테스트

#### 파일 수정
- `Engine/Source/Game/Actor/Private/CameraTestActor.cpp`

#### 수정 내용
```cpp
// CameraTestActor.cpp
void ACameraTestActor::RunAutomatedTests(float DeltaTime)
{
    // 기존 Vignette 테스트 유지
    // ...

    // ===== Camera Shake Bezier Curve 테스트 추가 =====
    static float ShakeTestTimer = 0.0f;
    ShakeTestTimer += DeltaTime;

    constexpr float ShakeTestInterval = 10.0f;

    if (ShakeTestTimer >= ShakeTestInterval)
    {
        ShakeTestTimer = 0.0f;

        APlayerCameraManager* CameraManager = GetCameraManager();
        if (CameraManager)
        {
            UCameraModifier_CameraShake* ShakeMod =
                Cast<UCameraModifier_CameraShake>(
                    CameraManager->FindCameraModifierByClass(
                        UCameraModifier_CameraShake::StaticClass()
                    )
                );

            if (ShakeMod)
            {
                static int32 ShakePhase = 0;
                ShakePhase = (ShakePhase + 1) % 4;

                switch (ShakePhase)
                {
                case 0: // Linear decay
                    ShakeMod->StartShakeWithCurve(3.0f, 10.0f, 5.0f,
                        ECameraShakePattern::Perlin, 15.0f,
                        FCubicBezierCurve::Linear());
                    UE_LOG_SYSTEM("Shake: Linear decay");
                    break;

                case 1: // Ease In
                    ShakeMod->StartShakeWithCurve(3.0f, 10.0f, 5.0f,
                        ECameraShakePattern::Sine, 10.0f,
                        FCubicBezierCurve::EaseIn());
                    UE_LOG_SYSTEM("Shake: Ease In");
                    break;

                case 2: // Ease Out
                    ShakeMod->StartShakeWithCurve(3.0f, 10.0f, 5.0f,
                        ECameraShakePattern::Perlin, 20.0f,
                        FCubicBezierCurve::EaseOut());
                    UE_LOG_SYSTEM("Shake: Ease Out");
                    break;

                case 3: // Bounce
                    ShakeMod->StartShakeWithCurve(3.0f, 10.0f, 5.0f,
                        ECameraShakePattern::Sine, 15.0f,
                        FCubicBezierCurve::Bounce());
                    UE_LOG_SYSTEM("Shake: Bounce");
                    break;
                }
            }
        }
    }
}
```

#### 체크리스트
- [ ] CameraTestActor.cpp 수정
- [ ] RunAutomatedTests()에 Shake 테스트 로직 추가
- [ ] 10초마다 다른 Bezier curve로 테스트
- [ ] PIE 모드에서 실행 확인
- [ ] 로그 출력 확인
- [ ] 각 커브의 감쇠 패턴 시각적 확인

---

## Phase 6: 문서화 및 최적화

### 6.1 코드 문서화 ⬜

**목표**: 주석 및 API 문서 작성

#### 작업 내용
- [ ] 모든 public 메서드에 Doxygen 주석 추가
- [ ] Bezier curve 프리셋 설명 추가
- [ ] CameraShakeDetailPanel 사용 가이드 작성
- [ ] README 또는 별도 문서 작성

---

### 6.2 성능 최적화 ⬜

**목표**: UI 렌더링 성능 개선

#### 최적화 항목
- [ ] 미리보기 그래프 샘플 수 조정 (200 → 최적값 찾기)
- [ ] Bezier curve 계산 캐싱 (변경 없으면 재계산 안 함)
- [ ] ImGui draw call 최적화
- [ ] 프로파일링 (필요 시)

---

## 구현 순서 (권장)

```
[1단계] Phase 1.1 → Phase 1.2
   └─> Bezier curve 기본 시스템 완성 후 빌드 테스트

[2단계] Phase 2.1
   └─> ImGui editor 단독 테스트 (간단한 테스트 윈도우)

[3단계] Phase 3.1 → Phase 3.2
   └─> Detail 패널 완성 및 Editor 통합

[4단계] Phase 4.1 → Phase 4.2
   └─> 전체 빌드 및 컴파일 오류 수정

[5단계] Phase 5.1 → Phase 5.2
   └─> 기능 검증 및 버그 수정

[6단계] Phase 6 (선택)
   └─> 문서화 및 성능 개선
```

---

## 전체 체크리스트

### Phase 1
- [ ] FCubicBezierCurve 구조체 생성
- [ ] UCameraModifier_CameraShake에 Bezier curve 통합

### Phase 2
- [ ] FImGuiBezierEditor 구현

### Phase 3
- [ ] FCameraShakeDetailPanel 구현
- [ ] Editor 윈도우 통합

### Phase 4
- [ ] 프로젝트 파일 업데이트
- [ ] 빌드 성공 확인

### Phase 5
- [ ] 기능 테스트 (16개 시나리오)
- [ ] CameraTestActor 통합 테스트

### Phase 6
- [ ] 문서화
- [ ] 성능 최적화

---

## 참고 자료

- **GitHub Issue #786**: https://github.com/ocornut/imgui/issues/786
- **Bezier Curve 수학**: https://robnapier.net/faster-bezier
- **Easing Functions**: https://easings.net/

---

## 작업 진행 상황

**시작일**: TBD
**완료일**: TBD
**현재 Phase**: Phase 1
**전체 진행률**: 0%

---

**작성자**: Claude Code
**마지막 업데이트**: 2025-01-05
