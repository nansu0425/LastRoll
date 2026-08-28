# 카메라 시스템 — CameraModifier 스택, 쉐이크 프리셋, 트랜지션

> 게임잼 주차(2025-10-31 ~ 11-06) 작업. 이 문서가 다루는 코드는 사실상 전부 본인 단독 작업입니다 (파일별 지분은 [Contribution.md](Contribution.md) 참고).

## 문제

게임에 필요한 카메라 요구사항이 세 가지였습니다.

1. 플레이어 피격·폭발 시 **카메라 쉐이크** — 디자이너(팀원)가 코드 수정 없이 강도·패턴·감쇠를 튜닝할 수 있어야 함
2. 게임 시작 시 **연출 카메라 → 게임플레이 카메라 트랜지션**
3. 두 효과가 **동시에** 걸려도 서로 간섭하지 않을 것

엔진(FutureEngine)에는 당시 "CameraComponent가 View 행렬을 만든다" 수준의 기능만 있었고, 카메라 효과를 얹을 구조가 없었습니다.

## 설계 — Unreal Engine의 APlayerCameraManager 패턴

효과마다 카메라 코드를 직접 고치는 대신, UE의 `APlayerCameraManager` + `UCameraModifier` 구조를 축소 구현했습니다. 핵심 아이디어는 **"카메라의 최종 상태(POV)는 매 프레임 파이프라인이 새로 만든다"** 입니다.

```
UWorld::Tick (모든 Actor Tick 이후)
  └─ APlayerCameraManager::UpdateCamera
       1. UpdateViewTarget      ─ ViewTarget(CameraComponent)에서 POV를 새로 읽음
       2. UpdateBlending        ─ ViewTarget 교체 시 이전/현재 POV 보간
       3. ApplyCameraModifiers  ─ modifier들을 priority 순으로 POV에 적용
       4. UpdateFading          ─ 화면 페이드 알파 갱신
       5. UpdateCameraConstants ─ 최종 POV → View/Projection 행렬
```

- POV(`FMinimalViewInfo`)는 Location / Rotation / FOV / Near·Far 등 카메라 상태의 값 객체이고, **매 프레임 ViewTarget에서 다시 만들어집니다**. 따라서 modifier가 어떤 값을 더하든 다음 프레임에 자동으로 초기화되고, 효과 간 상태 오염이 구조적으로 불가능합니다.
- 각 modifier는 `ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)` 하나만 구현합니다. 자신의 blend alpha(`AlphaInTime`/`AlphaOutTime`)를 갖고 있어 켜고 끌 때 부드럽게 페이드됩니다.
- 적용 순서는 priority 오름차순 정렬입니다. Transition(128)이 먼저 POV를 보간 값으로 만들고, CameraShake(200)가 그 위에 오프셋을 얹습니다. 요구사항 3(동시 적용)이 이 정렬 하나로 해결됩니다.

파이프라인 본체는 위 다이어그램과 1:1로 대응합니다:

```cpp
void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
	// 1단계: 현재 ViewTarget에서 POV 업데이트
	UpdateViewTarget(DeltaTime);

	// 2단계: 블렌딩 중이면 ViewTarget 간 보간
	if (bIsBlending)
	{
		UpdateBlending(DeltaTime);
	}

	// 3단계: 카메라 모디파이어 체인 적용
	ApplyCameraModifiers(DeltaTime);

	// 4단계: 페이딩 업데이트
	if (bIsFading)
	{
		UpdateFading(DeltaTime);
	}

	// 5단계: 최종 POV를 카메라 상수(View/Projection 행렬)로 변환
	UpdateCameraConstants();
}
```

modifier 합성은 정렬 → alpha 갱신 → 적용 순서입니다. modifier가 `ModifyCamera`에서 무엇을 하든 다음 프레임의 1단계가 POV를 새로 만들기 때문에, 여기서 상태를 되돌릴 필요가 없습니다:

```cpp
void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime)
{
	if (ModifierList.empty())
		return;

	// 우선순위로 모디파이어 정렬 (오름차순: 낮은 우선순위 먼저)
	std::sort(ModifierList.begin(), ModifierList.end(),
		[](const UCameraModifier* A, const UCameraModifier* B) {
			return A->GetPriority() < B->GetPriority();
		});

	// 각 모디파이어를 순서대로 적용
	for (UCameraModifier* Modifier : ModifierList)
	{
		if (Modifier && !Modifier->IsDisabled())
		{
			// 블렌드 알파 업데이트
			Modifier->UpdateAlpha(DeltaTime);

			// POV에 모디파이어 적용
			if (Modifier->GetAlpha() > 0.0f)
			{
				Modifier->ModifyCamera(DeltaTime, CachedPOV);
			}
		}
	}
}
```

관련 소스:

- [`Engine/Source/Actor/Private/PlayerCameraManager.cpp`](../Engine/Source/Actor/Private/PlayerCameraManager.cpp) — 파이프라인 본체
- [`Engine/Source/Component/Camera/Private/CameraModifier.cpp`](../Engine/Source/Component/Camera/Private/CameraModifier.cpp) — modifier 베이스 (alpha 상태 기계)
- [`Engine/Source/Component/Camera/Private/CameraModifier_CameraShake.cpp`](../Engine/Source/Component/Camera/Private/CameraModifier_CameraShake.cpp)
- [`Engine/Source/Component/Camera/Private/CameraModifier_Transition.cpp`](../Engine/Source/Component/Camera/Private/CameraModifier_Transition.cpp)

## 카메라 쉐이크 — 패턴 × 베지어 감쇠 커브 × 프리셋

쉐이크는 keyframe이 아니라 **절차적 oscillation**입니다. 위치·회전 진폭에 패턴 함수를 곱해 프레임마다 오프셋을 만듭니다.

- **패턴 3종**: `Sine`(축별 위상 오프셋), `Perlin`(자체 구현 1D Perlin noise — 정수 해시 + smoothstep 보간, 프레임 간 스무딩), `Random`(uniform 지터)
- **감쇠(decay)**: 쉐이크가 시간에 따라 잦아드는 곡선을 **cubic bezier curve**로 정의합니다. 커브를 안 쓰면 smoothstep 폴백. 최종 오프셋은 `패턴 × 감쇠 × modifier alpha`.
- **프리셋**: `{이름, 지속시간, 진폭, 패턴, 주파수, 감쇠 커브(P0~P3)}`를 JSON 파일(`CameraShakePresets.json`)로 저장·로드합니다. 게임 코드는 `PlayCameraShakePreset("Explosion")` 한 줄만 호출합니다.

`패턴 × 감쇠 × alpha` 합성은 `UCameraModifier_CameraShake::ModifyCamera`에 있습니다:

```cpp
// 감쇠 알파 계산 (시간에 따라 흔들림 강도 페이드 아웃)
float DecayAlpha;
if (bUseDecayCurve)
{
	// Bezier 곡선 기반 감쇠
	// X = 정규화된 경과 시간 [0,1], Y = 진폭 배율 [0,1]
	float NormalizedTime = 1.0f - (ShakeTimeRemaining / ShakeDuration); // 0(시작) → 1(끝)
	DecayAlpha = DecayCurve.SampleY(NormalizedTime);
}
else
{
	// 기존 방식: smoothstep 사용
	DecayAlpha = ShakeTimeRemaining / ShakeDuration;
	DecayAlpha = SmoothStep(DecayAlpha); // 부드러운 감쇠 곡선
}

// 흔들림 오프셋 평가
FVector LocationOffset = FVector::ZeroVector();
FVector RotationOffset = FVector::ZeroVector();
EvaluateShake(LocationOffset, RotationOffset, ShakeTime, DecayAlpha);

// 알파 블렌드 가중치 적용
float BlendWeight = GetAlpha();
LocationOffset *= BlendWeight;
RotationOffset *= BlendWeight;

// 위치 오프셋 적용 (월드 공간)
InOutPOV.Location += LocationOffset;
```

패턴 함수는 `EvaluateShake`의 switch 분기입니다. Perlin 케이스와, 케이스 공통으로 마지막에 곱해지는 감쇠:

```cpp
case ECameraShakePattern::Perlin:
{
	// 펄린 노이즈 (유기적이고 자연스러운 흔들림)
	// 각 축에 다른 오프셋으로 펄린 노이즈 샘플링
	float Speed = Frequency * 0.5f; // 펄린 주파수 제어

	OutLocationOffset.X = PerlinNoise1D(PerlinOffset.X + CurrentTime * Speed) * LocationAmplitude;
	OutLocationOffset.Y = PerlinNoise1D(PerlinOffset.Y + CurrentTime * Speed) * LocationAmplitude;
	OutLocationOffset.Z = PerlinNoise1D(PerlinOffset.Z + CurrentTime * Speed) * LocationAmplitude;

	// ... (회전 3축 동일 패턴)

	// 이전 프레임과 블렌드하여 부드러운 전환
	float SmoothFactor = 0.3f;
	OutLocationOffset = OutLocationOffset * (1.0f - SmoothFactor) + LastLocationOffset * SmoothFactor;
	OutRotationOffset = OutRotationOffset * (1.0f - SmoothFactor) + LastRotationOffset * SmoothFactor;

	LastLocationOffset = OutLocationOffset;
	LastRotationOffset = OutRotationOffset;
	break;
}
```

```cpp
// 감쇠 적용 (시간에 따라 페이드 아웃)
OutLocationOffset *= DecayAlpha;
OutRotationOffset *= DecayAlpha;
```

### ImGui 베지어 에디터

감쇠 커브를 수치로 튜닝하는 건 비효율적이라, **제어점 4개를 마우스로 드래그하는 커브 에디터**를 ImGui 위젯으로 직접 만들었습니다.

- 편집: P0.x=0, P3.x=1로 고정하고 나머지는 자유. **Y는 의도적으로 클램프하지 않아** 1.0을 넘는 오버슈트 커브(Bounce)도 만들 수 있습니다.
- 평가: 화면에는 t를 64분할한 폴리라인으로 그리고, 런타임 샘플링은 `SampleY(x)` — cubic bezier는 x→y 함수가 아니므로 **Newton-Raphson으로 x에서 t를 역산**한 뒤 y를 계산합니다.
- 에디터 윈도우([`CameraShakePresetEditorWindow.cpp`](../Engine/Source/Render/UI/Window/Private/CameraShakePresetEditorWindow.cpp))에서 프리셋 목록·커브 편집·저장을 한 화면에서 하고, PIE 실행 중 버튼 한 번으로 실제 게임 카메라에 즉시 재생해 확인할 수 있습니다.

x→t 역산은 [`BezierCurve.cpp`](../Engine/Source/Global/BezierCurve.cpp)에 있습니다:

```cpp
float FCubicBezierCurve::SampleY(float x, int32 iterations) const
{
	// Clamp x to [0, 1]
	x = Clamp(x, 0.0f, 1.0f);

	// Newton-Raphson 방법으로 X에 대응하는 t 찾기
	float t = SolveForT(x, iterations);

	// t로부터 Y 계산
	return BezierY(t);
}

float FCubicBezierCurve::SolveForT(float x, int32 iterations) const
{
	// Newton-Raphson 방법으로 X에 대응하는 t를 찾는다
	// f(t) = BezierX(t) - x = 0을 풀기

	// 초기 추정: linear approximation
	float t = x;

	for (int32 i = 0; i < iterations; ++i)
	{
		float currentX = BezierX(t);
		float derivative = BezierXDerivative(t);

		// 미분값이 0에 가까우면 더 이상 개선 불가
		if (std::abs(derivative) < 1e-6f)
			break;

		// Newton-Raphson: t_new = t_old - f(t) / f'(t)
		float error = currentX - x;
		t -= error / derivative;

		// t를 [0, 1] 범위로 clamp
		t = Clamp(t, 0.0f, 1.0f);

		// 수렴 체크 (오차가 충분히 작으면 종료)
		if (std::abs(error) < 1e-6f)
			break;
	}

	return t;
}
```

관련 소스: [`ImGuiBezierEditor.cpp`](../Engine/Source/ImGui/ImGuiBezierEditor.cpp), [`BezierCurve.cpp`](../Engine/Source/Global/BezierCurve.cpp), [`CameraShakePresetManager.cpp`](../Engine/Source/Manager/Camera/Private/CameraShakePresetManager.cpp)

## 카메라 트랜지션

시작 POV와 목표 POV 사이를 지정 시간 동안 보간하는 modifier입니다.

- Location은 Lerp, **Rotation은 quaternion Slerp**, FOV/Near/Far도 보간
- 진행률→보간 비율 매핑에 쉐이크와 같은 cubic bezier timing curve를 재사용 (EaseInOut 등)
- 트랜지션도 프리셋(JSON)으로 관리 — 게임 시작 연출은 `"Cinematic"`(3초, EaseInOut)을 씁니다

`UCameraModifier_Transition::ModifyCamera`의 보간부입니다:

```cpp
// Compute normalized time [0, 1]
float NormalizedTime = TransitionTime / TransitionDuration;

// Clamp to [0, 1] for safety
NormalizedTime = Clamp(NormalizedTime, 0.0f, 1.0f);

// Compute blend alpha from timing curve
float BlendAlpha = ComputeBlendAlpha(NormalizedTime);

// Interpolate POV
InOutPOV.Location = Lerp(StartPOV.Location, TargetPOV.Location, BlendAlpha);

// IMPORTANT: Use Quaternion Slerp for smooth rotation
InOutPOV.Rotation = FQuaternion::Slerp(StartPOV.Rotation, TargetPOV.Rotation, BlendAlpha);

InOutPOV.FOV = Lerp(StartPOV.FOV, TargetPOV.FOV, BlendAlpha);
InOutPOV.AspectRatio = Lerp(StartPOV.AspectRatio, TargetPOV.AspectRatio, BlendAlpha);
InOutPOV.NearClipPlane = Lerp(StartPOV.NearClipPlane, TargetPOV.NearClipPlane, BlendAlpha);
InOutPOV.FarClipPlane = Lerp(StartPOV.FarClipPlane, TargetPOV.FarClipPlane, BlendAlpha);
```

## 사례: 시작 연출이 끝나는 순간 카메라가 튀는 버그

게임 시작 연출은 "플레이어 근접 뷰 → 탑다운 게임플레이 뷰"로 3초 트랜지션합니다. 그런데 **트랜지션이 끝나는 프레임에 카메라가 순간적으로 다른 위치로 튀는(snap)** 문제가 있었습니다.

**원인** — 트랜지션의 목표 POV를 `TopDownCameraActor`의 **actor location**으로 잡고 있었습니다. 하지만 실제 렌더링 카메라는 actor가 아니라 그 밑에 붙은 `SpringArm → CameraComponent` 계층의 **CameraComponent world location**입니다. SpringArm이 `-Forward × ArmLength` 등의 오프셋을 더하므로 두 좌표는 다릅니다. 트랜지션 동안은 modifier가 POV를 직접 덮어쓰니 목표 지점(actor 위치)으로 잘 이동하지만, 트랜지션이 끝나 modifier가 꺼지는 순간 POV가 ViewTarget(CameraComponent) 기준으로 되돌아가면서 오프셋만큼 튀는 것입니다.

**해결** (커밋 `89d1053`) — 목표 POV를 CameraComponent의 world transform에서 취하도록 고쳤습니다. 연출 스크립트가 Lua라서, `UCameraComponent`의 `GetWorldLocation`/`GetWorldRotation`을 Lua에 바인딩하는 작업이 함께 들어갔습니다.

```lua
-- GameManager.lua — 수정 후
local cameraComp = camera:GetCameraComponent()
targetPOV.Location = cameraComp:GetWorldLocation()   -- SpringArm offset이 반영된 실제 카메라 위치
targetPOV.Rotation = cameraComp:GetWorldRotation()
```

트랜지션 종료 지점과 게임플레이 카메라 시작 지점이 같은 좌표가 되면서 snap이 사라졌습니다.

## 한계

- ViewTarget 교체 blending의 회전 보간이 Slerp가 아니라 성분별 Lerp입니다 (Transition modifier는 Slerp를 씀 — 회전 각이 작아 체감 문제는 없었지만 일관성이 없음)
- modifier 리스트를 매 프레임 정렬합니다. modifier가 2~3개라 실측 문제는 없지만, 리스트 변경 시에만 정렬하는 게 맞는 구조입니다
- 같은 클래스의 modifier를 1개만 운용하므로 서로 다른 쉐이크 2개를 동시에 재생할 수 없습니다
- SpringArm의 lag 상태(`LagLocation`/`LagRotation`)가 스폰 직후 원점/identity에서 시작하고 수렴이 틱당 고정 비율(framerate 의존)이라, 낮은 fps에서는 트랜지션 3초 안에 수렴하지 못한 잔차만큼 종료 순간 카메라가 튕겼다가 돌아오는 현상이 있었습니다 (60fps 기준 위치 ~0.7유닛 + pitch ~2°, 높은 fps에서는 소멸). 잼 이후 첫 틱에 lag 상태를 실제 pose로 스냅하도록 수정해 제거했습니다

## 당시 작업 문서

구현 전에 쓴 설계 문서와 정리 문서입니다.

- [PlayerCameraManager_Implementation_Plan.md](PlayerCameraManager_Implementation_Plan.md)
- [CameraTransition_Implementation_Plan.md](CameraTransition_Implementation_Plan.md)
- [BezierCurveEditor_CameraShake_Implementation_Plan.md](BezierCurveEditor_CameraShake_Implementation_Plan.md)
- [CameraShakePresetSystem_Implementation_Plan.md](CameraShakePresetSystem_Implementation_Plan.md)
- [CameraSystem_FrameFlow.md](CameraSystem_FrameFlow.md)
