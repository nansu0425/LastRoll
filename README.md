# Last Roll

**FutureEngine**(팀 자체 제작 DirectX 11 엔진) 위에서 만든 **탑다운 슈팅 서바이벌 게임**입니다.

플레이어는 굴러다니는 주사위를 조작하고, 몰려오는 당구공·체스말 적을 3종 투사체로 상대하며 최대한 오래 생존합니다. 생존 시간과 처치 수로 점수가 매겨집니다.

<!-- TODO: 플레이 영상 / GIF -->
<!-- TODO: 스크린샷 -->

- **기간** — 2025-10-31 ~ 11-06 (7일)
- **과정** — KRAFTON 정글 게임테크랩 2기 WEEK09 게임잼
- **팀** — 3명 (본인 포함)
- **본인 커밋** — 118건 (팀 전체 250건 중)

> **커밋 이력에 대해** — 이 저장소의 이력은 게임잼 이전 주차까지 이어지지만, 엔진 개발 시작(2025-09-02)부터의 전체 이력은 **아닙니다**. 2025-10-10에 당시 팀이 저장소를 새로 만들면서 그때까지의 코드베이스 322개 파일을 한 커밋으로 임포트했고, 그 이전 이력은 여기에 없습니다.
>
> 따라서 이력 구간은 **WEEK06 직전(2025-10-10) ~ WEEK09+**이며, 매 주차 팀이 새로 짜였으므로 **기여자 14명은 게임잼 팀이 아니라 이 구간 여러 주차 팀의 누적**입니다.

---

## ⚠️ 이 저장소에 대해

**소스 코드 공개용 저장소입니다. 빌드되지 않습니다.**

원본 팀 저장소에서 **코드만 추출**했습니다. 아래는 라이선스상 재배포할 수 없어 전부 제외했습니다.

| 제외 | 이유 |
|---|---|
| 모델 · 텍스처 · 폰트 · 오디오 | Quixel Megascans, Sketchfab, PUBG 공식 키비주얼, Unreal Engine 에디터 아이콘 등 재배포 불가 |
| 서드파티 라이브러리 (DirectXTK, Lua, Sol2, nlohmann, Dear ImGui) | 각 upstream에서 받는 것이 원칙 |

**원본 저장소의 커밋 히스토리와 기여자 정보는 그대로 보존했습니다** — 830 커밋, 14명 전원. 필터링으로 내용이 비게 된 커밋도 메시지·작성자·날짜를 남겨두었습니다.

---

## 내가 만든 것

두 갈래입니다. 이 게임을 만들며 작업한 것과, **게임잼 이전 주차에 만들어 이 게임에 그대로 쓰인 엔진 기능**입니다.

수치는 모두 `git blame -w` 기준 **현재 코드에 남아 있는 줄 수 / 파일 전체 줄 수**입니다. 커밋만 세는 것과 달리, 이후 팀원이 고쳐 쓴 부분은 빠집니다.

### 1. 게임 개발 (10-31 ~ 11-06, 본인 118 커밋 / 팀 3명 · 250 커밋)

**Lua(Sol2) 스크립팅 시스템** — 게임 로직 전체를 Lua로 작성할 수 있게 한 런타임

| 파일 | 남은 줄 |
|---|---:|
| `Source/Manager/Script/Private/ScriptManager.cpp` | 1,143 / 1,615 (70%) |
| `Source/Component/Private/ScriptComponent.cpp` | 355 / 572 (62%) |
| `Source/Component/Public/ScriptComponent.h` | 186 / 215 (86%) |
| `Source/Manager/Script/Public/ScriptManager.h` | 114 / 156 (73%) |

메타테이블 프록시로 C++ 객체를 Lua에 노출하고, 파일 변경을 감지해 **스크립트 핫 리로드**를 지원합니다. 리로드 실패 시 이전 상태로 롤백합니다.

**충돌 / Shape 컴포넌트** — 1,496 / 1,976줄 (75%)

`BoxComponent` 167/167, `CapsuleComponent` 144/144, `Capsule` 163/163, `BoundingCapsule` 163/163 — 전부 100%. Overlap 이벤트를 Lua 콜백으로 전달합니다.

**투사체 3종** — 272 / 272줄 (100%)

`LinearProjectile`(직선), `HomingProjectile`(유도), `OrbitProjectile`(공전) C++ 액터와 대응 Lua 스크립트.

**게임 로직 Lua**

| 파일 | 남은 줄 |
|---|---:|
| `Data/Scripts/HomingProjectile.lua` | 555 / 593 (93%) |
| `Data/Scripts/Player.lua` | 536 / 700 (76%) |
| `Data/Scripts/OrbitProjectile.lua` | 189 / 189 (100%) |
| `Data/Scripts/LinearProjectile.lua` | 142 / 163 (87%) |

**카메라 시스템** — 6,316 / 6,357줄 (99%), 사실상 단독 작업

| 파일 | 남은 줄 |
|---|---:|
| `Source/Actor/Private/PlayerCameraManager.cpp` | 515 / 523 (98%) |
| `Source/Actor/Public/PlayerCameraManager.h` | 277 / 290 (95%) |
| `Source/Render/UI/Window/Private/CameraShakePresetEditorWindow.cpp` | 392 / 392 (100%) |
| `Source/Component/Camera/Private/CameraModifier_CameraShake.cpp` | 299 / 299 (100%) |
| `Source/Editor/Private/CameraShakePresetDetailPanel.cpp` | 295 / 295 (100%) |
| `Source/ImGui/ImGuiBezierEditor.cpp` | 255 / 255 (100%) |
| `Source/Manager/Camera/Private/CameraShakePresetManager.cpp` | 253 / 253 (100%) |
| `Source/Component/Camera/Private/CameraModifier_Transition.cpp` | 186 / 186 (100%) |
| `Source/Component/Camera/Private/CameraModifier.cpp` | 119 / 119 (100%) |

UE의 `APlayerCameraManager` 패턴을 따라 **CameraModifier 스택**으로 설계했습니다. 카메라 쉐이크와 트랜지션이 각각 모디파이어로 붙고, 쉐이크 커브는 직접 만든 **ImGui 베지어 에디터**로 편집한 뒤 프리셋으로 저장합니다.

### 2. 이 게임에 쓰인 엔진 기능 — WEEK08 섀도우 매핑 (10-24 ~ 10-30, 본인 29 커밋 / 팀 4명)

게임잼 이전 주차에 만든 것이 그대로 이 게임의 그림자를 그립니다. **2,880 / 3,706줄 (77%)**

| 파일 | 남은 줄 |
|---|---:|
| `Source/Render/Shadow/Private/PSMCalculator.cpp` | 718 / 718 (100%) |
| `Source/Render/Shadow/Private/PSMBounding.cpp` | 429 / 429 (100%) |
| `Source/Render/Shadow/Public/PSMBounding.h` | 226 / 226 (100%) |
| `Source/Render/Shadow/Public/PSMCalculator.h` | 145 / 145 (100%) |
| `Source/Render/RenderPass/Private/ShadowMapPass.cpp` | 843 / 1,390 (60%) |
| `Source/Render/RenderPass/Public/ShadowMapPass.h` | 160 / 283 (56%) |
| `Source/Texture/Public/ShadowMapResources.h` | 99 / 105 (94%) |
| `Source/Texture/Private/ShadowMapResources.cpp` | 158 / 224 (70%) |

Directional / Spot / Point 3종 광원의 섀도우 매핑을 구현하고, Directional에 **PSM(Perspective Shadow Mapping)** 과 LiSPSM을 적용했습니다. Point Light는 Cube Shadow Map을 씁니다.

### 설계 문서 (13,217 / 13,750줄, 96%)

구현과 함께 쓴 문서입니다.

- [`Document/DirectionalLight_ShadowMap.md`](Document/DirectionalLight_ShadowMap.md) — 1,135줄
- [`Document/PointLight_ShadowMap.md`](Document/PointLight_ShadowMap.md) — 1,309줄
- [`Document/SpotLight_ShadowMap.md`](Document/SpotLight_ShadowMap.md) — 1,234줄
- [`Document/PlayerCameraManager_Implementation_Plan.md`](Document/PlayerCameraManager_Implementation_Plan.md) — 1,537줄
- [`Document/CameraTransition_Implementation_Plan.md`](Document/CameraTransition_Implementation_Plan.md) — 1,522줄
- [`Document/CameraSystem_FrameFlow.md`](Document/CameraSystem_FrameFlow.md) — 989줄
- [`Document/BezierCurveEditor_CameraShake_Implementation_Plan.md`](Document/BezierCurveEditor_CameraShake_Implementation_Plan.md) — 926줄
- [`Document/CameraShakePresetSystem_Implementation_Plan.md`](Document/CameraShakePresetSystem_Implementation_Plan.md) — 897줄
- [`Document/LastRoll_Technical_Documentation.md`](Document/LastRoll_Technical_Documentation.md) — 1,528줄

---

## 팀원이 만든 것

이 저장소의 대부분은 제 코드가 아닙니다. 혼동을 막기 위해 **제가 만들지 않은 주요 기능**을 밝힙니다.

| 기능 | 제 지분 |
|---|---:|
| PCF 소프트 섀도우 (`ShadowMapFilterPass`) | 0 / 267 (0%) |
| BVH 가속 구조 (`Global/BVH.cpp`) | 0 / 481 (0%) |
| 라이트 컴포넌트 전반 | 282 / 1,676 (16%) |
| 에디터 UI 전반 | 1,377 / 10,364 (13%) |

섀도우 매핑은 제가 만들었지만 **PCF 필터는 이후 팀원이 새로 작성**했습니다. BVH는 처음부터 다른 팀원 작업입니다.

## 전체 지분

| 구분 | 남은 줄 | 비율 |
|---|---:|---:|
| 코드 (cpp/h/hlsl/lua) | 17,650 / 81,705 | 21.6% |
| 설계 문서 (md) | 13,217 / 13,750 | 96.1% |

---

## 원본 · 기여자

- 팀이 작성한 기존 최상위 README(WEEK08 기능 요약)는 [`Document/FutureEngine_WEEK08_Features.md`](Document/FutureEngine_WEEK08_Features.md) 로 옮겼습니다.
- 원본 저장소 — `nansu0425/GameTechLab-WEEK09-plus` (비공개 전환)
- 여러 주차 팀 기여자 14명의 커밋이 그대로 보존되어 있습니다: `git shortlog -sne`
- 주차별 팀·담당 영역은 [`nansu0425/KRAFTON-GameTechLab-Engine`](https://github.com/nansu0425/KRAFTON-GameTechLab-Engine)의 주차별 작업 문서를 참고하세요.
- KRAFTON 정글 게임테크랩 2기 교육과정 산출물이며, **포트폴리오 목적으로 코드만 공개**합니다.
- 별도 라이선스를 두지 않았습니다. 공동 저작물이므로 코드 재사용을 원하시면 문의해 주세요.
