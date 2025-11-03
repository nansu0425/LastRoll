-- ==============================================================================
-- OrbitProjectile.lua
-- ==============================================================================
-- Player 주변을 회전하는 궤도 투사체
-- Enemy와 충돌 시 데미지를 주지만 사라지지 않고 계속 회전
-- ==============================================================================

local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")

-- ==============================================================================
-- Configuration
-- ==============================================================================

local DefaultRadius = 5.0           -- 기본 회전 반경
local DefaultRotationSpeed = 90.0   -- 기본 회전 속도 (degree/s, 4초에 1바퀴)
local DefaultDamage = 5             -- 기본 데미지

-- ==============================================================================
-- Lifecycle Functions
-- ==============================================================================

function BeginPlay()
    obj.CenterActor = nil           -- 중심이 되는 Actor (Player)
    obj.Radius = DefaultRadius      -- 회전 반경
    obj.CurrentAngle = 0.0          -- 현재 각도 (degree)
    obj.RotationSpeed = DefaultRotationSpeed  -- 회전 속도 (degree/s)
    obj.Damage = DefaultDamage      -- 충돌 데미지
    obj.IsInitialized = false       -- 초기화 여부

    -- Overlap 델리게이트 바인딩
    local SphereComp = Owner:GetComponent("USphereComponent")
    if SphereComp then
        SphereComp:BindBeginOverlap(self, OnBeginOverlap)
        SphereComp:BindEndOverlap(self, OnEndOverlap)
    end
end

function EndPlay()
    -- Cleanup
end

-- ==============================================================================
-- Setup Function
-- ==============================================================================

---
-- 궤도 투사체 초기화 함수
-- @param InCenterActor: 중심이 되는 Actor (Player)
-- @param InInitialAngle: 초기 각도 (degree)
-- @param InRadius: 회전 반경
-- @param InSpeed: 회전 속도 (degree/s)
-- @param InDamage: 충돌 데미지
---
function Setup(InCenterActor, InInitialAngle, InRadius, InSpeed, InDamage)
    obj.CenterActor = InCenterActor
    obj.CurrentAngle = InInitialAngle or 0.0
    obj.Radius = InRadius or DefaultRadius
    obj.RotationSpeed = InSpeed or DefaultRotationSpeed
    obj.Damage = InDamage or DefaultDamage
    obj.IsInitialized = true

    -- 초기 위치 설정
    UpdatePosition()
end

-- ==============================================================================
-- Tick Function
-- ==============================================================================

function Tick(dt)
    -- 게임 상태 체크
    if _G.GameData.GameState == EGameState.End then
        ReturnToPool()
        return
    end
    if Util.IsActiveMode() == false then
        return
    end

    -- 초기화되지 않은 투사체는 동작하지 않음
    if not obj.IsInitialized then
        return
    end

    -- CenterActor가 사라졌으면 제거
    if not obj.CenterActor then
        ReturnToPool()
        return
    end

    -- 각도 업데이트 (회전)
    obj.CurrentAngle = obj.CurrentAngle + obj.RotationSpeed * dt

    -- 각도를 0~360 범위로 유지
    if obj.CurrentAngle >= 360.0 then
        obj.CurrentAngle = obj.CurrentAngle - 360.0
    elseif obj.CurrentAngle < 0.0 then
        obj.CurrentAngle = obj.CurrentAngle + 360.0
    end

    -- 위치 업데이트 (원운동)
    UpdatePosition()
end

-- ==============================================================================
-- Helper Functions
-- ==============================================================================

---
-- 원운동 위치 계산 및 적용
---
function UpdatePosition()
    if not obj.CenterActor then
        return
    end

    local CenterPos = obj.CenterActor.Location
    local AngleRad = math.rad(obj.CurrentAngle)

    -- 원운동 공식: x = centerX + radius * cos(angle), y = centerY + radius * sin(angle)
    obj.Location = Vector(
        CenterPos.x + obj.Radius * math.cos(AngleRad),
        CenterPos.y + obj.Radius * math.sin(AngleRad),
        CenterPos.z  -- 같은 높이 유지
    )
end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

function OnBeginOverlap(OtherActor)
    if not obj.IsInitialized then
        return
    end

    -- 자신과의 충돌 무시
    if OtherActor.UUID == obj.UUID then
        return
    end

    -- CenterActor와의 충돌 무시 (Player와는 충돌하지 않음)
    if obj.CenterActor and OtherActor.UUID == obj.CenterActor.UUID then
        return
    end

    -- Enemy와 충돌 체크
    local ScriptComp = OtherActor:GetScriptComponent()
    if not ScriptComp then
        return
    end

    local Env = ScriptComp:GetEnv()

    -- TakeDamage 함수가 있으면 Enemy로 판별
    if not Env["TakeDamage"] then
        return
    end

    -- 죽은 Enemy는 무시
    if Env["obj"] and Env["obj"].IsDead then
        return
    end

    -- 데미지 처리
    Env["TakeDamage"](obj.Damage)
end

function OnEndOverlap(OtherActor)
    -- Orbit projectile은 EndOverlap 처리 불필요
end

-- ==============================================================================
-- Pool Management
-- ==============================================================================

function ReturnToPool()
    -- 상태 초기화
    obj.IsInitialized = false
    obj.CenterActor = nil
    obj.CurrentAngle = 0.0
    obj.Radius = DefaultRadius
    obj.RotationSpeed = DefaultRotationSpeed
    obj.Damage = DefaultDamage

    -- ActorPool에 반납
    ActorPool:Return(Owner)
end
