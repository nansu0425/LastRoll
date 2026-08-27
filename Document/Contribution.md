# 기여 통계

이 저장소에서 본인(nansu0425)이 작성한 코드의 정량 지표입니다.

## 산정 방법

- `git blame -w --line-porcelain` 기준으로 **현재 코드에 남아 있는 본인 author 줄 수 / 파일 전체 줄 수**를 셉니다. 커밋 수를 세는 것과 달리, 이후 팀원이 고쳐 쓴 줄은 본인 지분에서 빠집니다.
- 대상: git이 추적하는 `*.cpp` `*.h` `*.hlsl` `*.lua`(코드), `*.md`(문서) 전체
- 산정 기준 커밋: `e7543d1` (2026-08-27, 섀도우 dead code 정리 반영 후)

## 전체 지분

| 구분 | 본인 줄 / 전체 줄 | 비율 |
|---|---:|---:|
| 코드 (cpp/h/hlsl/lua) | 15,940 / 79,659 | 20.0% |
| 설계 문서 (`Document/*.md`) | 16,209 / 16,209 | 100% |

커밋 수 기준으로는 게임잼 주차(10-31 ~ 11-06) 팀 전체 250건 중 본인 118건입니다.

## 게임잼 주차 (2025-10-31 ~ 11-06, 팀 3명)

### Lua(Sol2) 스크립팅 시스템 — [기술 문서](Feature_LuaScripting.md)

| 파일 | 본인 줄 / 전체 줄 |
|---|---:|
| `Engine/Source/Manager/Script/Private/ScriptManager.cpp` | 1,143 / 1,615 (71%) |
| `Engine/Source/Component/Private/ScriptComponent.cpp` | 355 / 572 (62%) |
| `Engine/Source/Component/Public/ScriptComponent.h` | 186 / 215 (87%) |
| `Engine/Source/Manager/Script/Public/ScriptManager.h` | 114 / 156 (73%) |

### 카메라 시스템 — [기술 문서](Feature_CameraSystem.md)

| 파일 | 본인 줄 / 전체 줄 |
|---|---:|
| `Engine/Source/Actor/Private/PlayerCameraManager.cpp` | 515 / 523 (98%) |
| `Engine/Source/Actor/Public/PlayerCameraManager.h` | 277 / 290 (96%) |
| `Engine/Source/Component/Camera/Private/CameraModifier.cpp` | 119 / 119 (100%) |
| `Engine/Source/Component/Camera/Private/CameraModifier_CameraShake.cpp` | 299 / 299 (100%) |
| `Engine/Source/Component/Camera/Private/CameraModifier_Transition.cpp` | 186 / 186 (100%) |
| `Engine/Source/Global/BezierCurve.cpp` | 226 / 226 (100%) |
| `Engine/Source/ImGui/ImGuiBezierEditor.cpp` | 255 / 255 (100%) |
| `Engine/Source/Manager/Camera/Private/CameraShakePresetManager.cpp` | 253 / 253 (100%) |
| `Engine/Source/Manager/Camera/Private/TransitionPresetManager.cpp` | 242 / 242 (100%) |
| `Engine/Source/Render/UI/Window/Private/CameraShakePresetEditorWindow.cpp` | 392 / 392 (100%) |
| `Engine/Source/Editor/Private/CameraShakePresetDetailPanel.cpp` | 295 / 295 (100%) |
| `Engine/Source/Game/Actor/Private/TopDownCameraActor.cpp` | 110 / 116 (95%) |

(각 cpp의 대응 헤더도 대부분 100%입니다)

### 충돌 / Shape 컴포넌트

| 파일 | 본인 줄 / 전체 줄 |
|---|---:|
| `Engine/Source/Component/Shape/Private/BoxComponent.cpp` | 167 / 167 (100%) |
| `Engine/Source/Component/Shape/Private/CapsuleComponent.cpp` | 144 / 144 (100%) |
| `Engine/Source/Component/Shape/Private/SphereComponent.cpp` | 130 / 153 (85%) |
| `Engine/Source/Physics/Private/Capsule.cpp` | 163 / 163 (100%) |
| `Engine/Source/Physics/Private/BoundingCapsule.cpp` | 163 / 163 (100%) |

### 투사체 3종 — C++ 액터 + Lua 스크립트

C++ 액터(`LinearProjectile`, `HomingProjectile`, `OrbitProjectile` cpp/h 6개 파일)는 272 / 272 (100%).

| 파일 | 본인 줄 / 전체 줄 |
|---|---:|
| `Engine/Data/Scripts/HomingProjectile.lua` | 555 / 593 (94%) |
| `Engine/Data/Scripts/Player.lua` | 536 / 700 (77%) |
| `Engine/Data/Scripts/OrbitProjectile.lua` | 189 / 189 (100%) |
| `Engine/Data/Scripts/LinearProjectile.lua` | 142 / 163 (87%) |

### Emissive 지원 — [기술 문서](Feature_EmissiveProjectile.md)

Material·constant buffer·셰이더에 걸쳐 얇게 퍼진 작업이라 줄 수는 작습니다: `Material.h` 6줄, `StaticMeshPass.cpp` 18줄, `UberLit.hlsl` 117줄(emissive 항 + 섀도우 샘플링 수정 포함), `OrbitProjectile.cpp` 91 / 91 (100%).

## 엔진 개발 주차 — WEEK08 섀도우 매핑 (2025-10-24 ~ 10-30, 팀 4명) — [기술 문서](Feature_ShadowMapping.md)

| 파일 | 본인 줄 / 전체 줄 |
|---|---:|
| `Engine/Source/Render/RenderPass/Private/ShadowMapPass.cpp` | 773 / 1,191 (65%) |
| `Engine/Source/Render/RenderPass/Public/ShadowMapPass.h` | 155 / 240 (65%) |
| `Engine/Source/Texture/Private/ShadowMapResources.cpp` | 158 / 224 (71%) |
| `Engine/Source/Texture/Public/ShadowMapResources.h` | 99 / 105 (94%) |

Shadow filtering(`ShadowMapFilterPass`)과 CSM(`CascadeManager`)은 팀원 작업입니다.

## 설계 문서 (`Document/`, 전부 본인 작성)

구현 전·중에 쓴 작업 문서들입니다. 면접관용으로 정리한 기술 문서(`Feature_*.md`)와 달리 당시 작업 기록 그대로입니다.

- 섀도우: `DirectionalLight_ShadowMap.md`(1,135줄) · `SpotLight_ShadowMap.md`(1,234줄) · `PointLight_ShadowMap.md`(1,309줄)
- 카메라: `PlayerCameraManager_Implementation_Plan.md`(1,537줄) · `CameraTransition_Implementation_Plan.md`(1,522줄) · `CameraSystem_FrameFlow.md`(989줄) · `BezierCurveEditor_CameraShake_Implementation_Plan.md`(926줄) · `CameraShakePresetSystem_Implementation_Plan.md`(897줄)
- 기타: `LastRoll_Technical_Documentation.md`(1,528줄) 등

## 커밋 이력에 대한 주의사항

- 이 저장소의 commit history는 게임잼 이전 주차까지 이어지지만, 엔진 개발 시작(2025-09-02)부터의 전체 history는 **아닙니다**. 2025-10-10에 당시 팀이 저장소를 새로 만들면서 그때까지의 코드베이스 322개 파일을 한 커밋으로 임포트했고, 그 이전 이력은 여기에 없습니다.
- contributors는 게임잼 팀(3명)뿐 아니라 **엔진 개발에 참여했던 모든 인원**을 포함합니다.
- 게임잼 종료(2025-11-06) 이후의 커밋은 포트폴리오 정리 목적입니다: README·문서 재구성, 로컬 빌드 환경 복원, 그리고 미완성 섀도우 시도 코드(PSM/LSPSM/TSM) 제거(`7fbfc49`). 이 정리로 본인 작성 줄 약 1,700줄이 삭제됐으므로, 위 통계는 정리 **후** 기준입니다.
