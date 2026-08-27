# Last Roll

KRAFTON 정글 게임테크랩 2기 9주차 게임잼에서 만든 **탑다운 슈팅 서바이벌 게임**입니다.

<!-- TODO: 플레이 영상 / GIF -->
<!-- TODO: 스크린샷 -->

- **게임잼**
    - 기간: 2025-10-31 ~ 11-06 (7일)
    - 팀: 3명
    - 게임 컨셉: 플레이어는 굴러다니는 주사위를 조작하고, 몰려오는 적을 투사체로 상대하며 오래 생존하는 것이 목표

- **엔진 개발**
    - 기간: 2025-09-02 ~ 10-30 (59일)
    - 팀: 매 주차 3~4명 팀으로 진행, 팀원은 매 주차 랜덤 변경
    - 개발 방식: 팀원 중 한 명의 전 주차 엔진을 골라 그 위에 새로운 feature 개발

## 핵심 작업

각 작업의 문제·설계·구현·한계는 링크된 기술 문서에 정리했습니다.

### [Lua(Sol2) 스크립팅 시스템](Document/Feature_LuaScripting.md) (WEEK09)

게임 로직 전체(플레이어·적·투사체·게임 매니저)를 Lua로 작성할 수 있게 한 런타임입니다. 스크립트 인스턴스마다 environment를 분리해 같은 스크립트를 쓰는 액터들이 독립 상태를 갖고, 메타테이블 프록시로 액터 속성과 스크립트 동적 속성을 하나의 `obj` 이름 공간으로 노출합니다. 모든 호출이 protected call을 경유해 스크립트 에러가 엔진을 죽이지 않으며, 에디터에서 파일 저장 시 0.5초 내에 핫 리로드됩니다.

### [카메라 시스템](Document/Feature_CameraSystem.md) (WEEK09)

UE의 `APlayerCameraManager` 패턴을 따라 **CameraModifier 스택**으로 설계했습니다. 카메라 쉐이크와 트랜지션이 각각 modifier로 붙어 priority 순으로 합성되므로 두 효과가 동시에 걸려도 간섭하지 않습니다. 쉐이크 감쇠 곡선은 직접 만든 **ImGui 베지어 에디터**로 편집해 JSON 프리셋으로 저장하고, PIE 실행 중 즉시 재생해 튜닝합니다. 시작 연출이 끝날 때 카메라가 튀던 버그를 SpringArm offset 좌표계 불일치로 진단하고 해결한 사례도 문서에 포함했습니다.

### [Emissive 미지원 엔진에서 빛나는 투사체](Document/Feature_EmissiveProjectile.md) (WEEK09)

엔진에 emissive도 bloom/HDR도 없는 상태에서 "빛나는 태양" 투사체를 만들기 위해, material→constant buffer→셰이더로 emissive 항을 새로 관통시키고, 공유 material을 오염시키지 않도록 per-instance material 복제로 적용한 뒤, 같은 색 PointLight를 부착해 주변 지오메트리가 실제로 빛을 받게 했습니다.

### [섀도우 매핑 — 3종 광원](Document/Feature_ShadowMapping.md) (WEEK08)

게임잼 이전 주차에 개발한 feature 입니다. Directional(씬 AABB 기반 orthographic) / Spot(cone frustum perspective) / Point(cube map 6면 + linear distance) 광원별 shadow map 생성 경로와, shadow acne 대응을 위한 DepthBias 기반 rasterizer state 캐싱을 구현했습니다. 이 중 게임에 실제로 사용된 것은 **Spot 경로**입니다 — 플레이어를 따라오는 SpotLight 3개가 이 경로로 그림자를 드리우며, Directional·Point shadow는 게임 씬에서 쓰이지 않았습니다.

### 그 외 (WEEK09)

- **충돌 / Shape 컴포넌트** — `BoxComponent` · `CapsuleComponent` · `SphereComponent`와 capsule 충돌 판정. Overlap 이벤트를 Lua 콜백으로 전달합니다 (전달 구조는 [Lua 문서](Document/Feature_LuaScripting.md) 참고)
- **투사체 3종** — `LinearProjectile`(직선) · `HomingProjectile`(유도) · `OrbitProjectile`(공전) C++ 액터와 대응 Lua 스크립트

## 기여 통계

파일별 `git blame` 기준 지분과 산정 방법은 [Document/Contribution.md](Document/Contribution.md)에 있습니다.

## 이 저장소에 대해

- **소스 코드 공개용 저장소입니다. 빌드되지 않습니다** — 원본 팀 저장소에서 코드만 추출했고, 재배포 시 문제 소지가 있는 에셋과 서드파티 라이브러리는 제외했습니다.
- 원본 저장소의 커밋 히스토리와 기여자 정보는 그대로 보존했습니다. 게임잼 종료 후에는 포트폴리오 정리 목적의 커밋(README·문서 재구성, 미완성 섀도우 시도 코드 제거)이 있습니다. 이력 관련 상세한 주의사항은 [Contribution.md](Document/Contribution.md#커밋-이력에-대한-주의사항)에 있습니다.
