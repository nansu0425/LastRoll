-- ==============================================================================
-- HomingProjectile.lua
-- ==============================================================================
-- 유도 투사체 행동 스크립트
-- 타겟 Enemy를 추적하여 자동으로 방향을 조정
-- ActorPool로 관리되는 재사용 가능한 투사체
-- ==============================================================================

local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")

-- ==============================================================================
-- Configuration
-- ==============================================================================

-- 투사체 기본 설정
local InitialSpeed = 6.0  -- 초기 속도 (천천히 시작)
local TargetReachTime = 0.6  -- 목표 도달 시간 (초) - 거리에 상관없이 이 시간 내에 도달
local DefaultDamage = 15
local DefaultMaxDistance = 100.0
local RotationSpeed = 3.5  -- 유도 강도 (낮춰서 빙글빙글 방지)
local MaxSpeed = 45.0  -- 최대 속도 제한 (낮춰서 tunneling 방지)

-- 거리 기반 설정
local VeryCloseDistance = 2.0  -- 즉시 충돌 처리 거리
local CloseDistance = 5.0  -- 가까운 거리 기준
local MediumDistance = 10.0  -- 중간 거리 기준
local CriticalDistance = 8.0  -- 급가속 시작 거리
local SlowdownDistance = 1.5  -- 감속 시작 거리
local ForceCollisionDistance = 1.2  -- 강제 충돌 체크 거리 (Overlap 없어도 충돌 처리)

-- 속도 제한
local CloseMaxSpeed = 18.0  -- 가까운 거리 최대 속도 (낮춤)
local MediumMaxSpeed = 28.0  -- 중간 거리 최대 속도 (낮춤)
local FarMaxSpeed = 45.0  -- 먼 거리 최대 속도 (낮춤)

-- 도달 시간
local CloseReachTime = 1.2  -- 가까운 거리 도달 시간
local MediumReachTime = 0.9  -- 중간 거리 도달 시간
local FarReachTime = 0.6  -- 먼 거리 도달 시간

-- 각도 기반 설정
local AngleThreshold = 0.85  -- 각도 보정 시작 threshold (약 30도)
local MinSpeedRatio = 0.3  -- 최소 속도 비율 (각도가 클 때)
local MinRotationBoost = 1.5  -- 최소 회전 속도 부스트 (낮춤)
local MaxRotationBoost = 3.0  -- 최대 회전 속도 부스트 (낮춰서 빙글빙글 방지)

-- ==============================================================================
-- Lifecycle Functions
-- ==============================================================================

function BeginPlay()
    obj.TargetActor = nil
    obj.TargetUUID = nil  -- Target의 UUID (절대 변경되지 않음)
    obj.TargetLocation = nil  -- 현재 추적 중인 위치
    obj.TargetLost = false  -- Target이 사라졌는지 여부
    obj.Speed = InitialSpeed  -- 초기 속도
    obj.Acceleration = 0.0  -- 가속도 (Setup에서 계산)
    obj.ElapsedTime = 0.0  -- 경과 시간
    obj.InitialDistance = 0.0  -- 초기 거리
    obj.DynamicMaxSpeed = MaxSpeed  -- 거리에 따른 동적 최대 속도
    obj.Damage = DefaultDamage
    obj.MaxDistance = DefaultMaxDistance
    obj.TraveledDistance = 0.0
    obj.Direction = Vector(1, 0, 0)
    obj.IsInitialized = false

    --print("HomingProjectile created: " .. obj.UUID)

    -- Overlap 델리게이트 바인딩
    local SphereComp = Owner:GetComponent("USphereComponent")
    if SphereComp then
        --print("[HomingProjectile] Binding overlap to SphereComponent")
        SphereComp:BindBeginOverlap(self, OnBeginOverlap)
        SphereComp:BindEndOverlap(self, OnEndOverlap)
    else
        --print("[HomingProjectile] WARNING: No SphereComponent found!")
    end
end

function EndPlay()
    --print("HomingProjectile ending: " .. obj.UUID)
end

-- ==============================================================================
-- Setup Function
-- ==============================================================================

---
-- 투사체 초기화 함수 (발사 시 호출)
-- @param InTargetActor: 추적할 타겟 Actor
-- @param InSpeed: 투사체 속도 (사용하지 않음 - 거리에 따라 동적 변경)
-- @param InDamage: 데미지 (선택)
-- @param InMaxDistance: 최대 이동 거리 (선택)
---
function Setup(InTargetActor, InSpeed, InDamage, InMaxDistance)
    obj.TargetActor = InTargetActor
    obj.TargetLost = false
    obj.Damage = InDamage or DefaultDamage
    obj.MaxDistance = InMaxDistance or DefaultMaxDistance
    obj.TraveledDistance = 0.0
    obj.ElapsedTime = 0.0

    if not obj.TargetActor then
        return
    end

    -- Target UUID와 초기 위치 저장
    obj.TargetUUID = obj.TargetActor.UUID
    obj.TargetLocation = obj.TargetActor.Location
    obj.InitialDistance = (obj.TargetLocation - obj.Location):Length()

    -- 매우 가까운 거리 체크 (즉시 충돌 처리)
    if HandleImmediateCollision() then
        return
    end

    -- 초기 방향 설정
    obj.Direction = obj.TargetActor.Location - obj.Location
    obj.Direction:Normalize()
    --print("[HomingProjectile] Target locked: " .. obj.TargetUUID)

    -- 거리에 따른 동적 매개변수 계산
    CalculateDynamicParameters()

    -- Target Enemy에 자신을 등록
    RegisterToTarget()

    obj.IsInitialized = true
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

    -- 초기화되지 않은 투사체는 이동하지 않음
    if not obj.IsInitialized then
        return
    end

    -- 타겟 상태 업데이트
    UpdateTargetStatus()

    -- TargetLocation을 향해 이동
    if not obj.TargetLocation then
        -- TargetLocation이 없으면 마지막 방향으로 직진
        ApplyMovement(dt)
        return
    end

    -- 타겟 방향 및 거리 계산
    local ToTarget = obj.TargetLocation - obj.Location
    local DistanceToTarget = ToTarget:Length()

    -- 강제 충돌 체크: 거리가 매우 가까우면 Overlap 이벤트 없어도 충돌 처리 (tunneling 방지)
    if not obj.TargetLost and obj.TargetActor and DistanceToTarget <= ForceCollisionDistance then
        --print("[HomingProjectile] Force collision triggered at distance: " .. DistanceToTarget)
        HandleTargetCollision(obj.TargetActor)
        return
    end

    -- 타겟 도달 체크 (타겟이 사라진 경우)
    if obj.TargetLost and DistanceToTarget < 1.0 then
        --print("[HomingProjectile] Reached target location, returning to pool")
        ReturnToPool()
        return
    end

    -- 속도 계산 및 적용
    CalculateAndApplySpeed(DistanceToTarget)

    -- 회전 계산 및 적용
    CalculateAndApplyRotation(ToTarget, DistanceToTarget, dt)

    -- 이동 적용
    ApplyMovement(dt)
end

-- ==============================================================================
-- Setup Helper Functions
-- ==============================================================================

---
-- 매우 가까운 거리일 때 즉시 충돌 처리
-- @return true if collision was handled immediately, false otherwise
---
function HandleImmediateCollision()
    if obj.InitialDistance >= VeryCloseDistance then
        return false
    end

    --print("[HomingProjectile] Target is very close (" .. obj.InitialDistance .. "), attempting immediate collision")

    local TargetScript = obj.TargetActor:GetScriptComponent()
    if not TargetScript then
        return false
    end

    local TargetEnv = TargetScript:GetEnv()
    if not (TargetEnv["TakeDamage"] and TargetEnv["RegisterProjectile"]) then
        return false
    end

    -- Enemy에 등록하고 즉시 데미지 처리
    TargetEnv["RegisterProjectile"](obj.UUID)

    if not TargetEnv["obj"].IsDead then
        TargetEnv["TakeDamage"](obj.Damage)
        --print("[HomingProjectile] Immediate damage dealt")
    end

    TargetEnv["UnregisterProjectile"](obj.UUID)

    -- 즉시 제거
    obj.IsInitialized = false
    ActorPool:Return(Owner)
    return true
end

---
-- 거리에 따른 동적 도달 시간 및 최대 속도 계산
---
function CalculateDynamicParameters()
    local DynamicReachTime, DynamicMaxSpeed

    if obj.InitialDistance < CloseDistance then
        -- 매우 가까운 거리: 천천히 도달
        DynamicReachTime = CloseReachTime
        DynamicMaxSpeed = CloseMaxSpeed
    elseif obj.InitialDistance < MediumDistance then
        -- 가까운 거리: 적당한 속도
        DynamicReachTime = MediumReachTime
        DynamicMaxSpeed = MediumMaxSpeed
    else
        -- 먼 거리: 빠르게 도달
        DynamicReachTime = FarReachTime
        DynamicMaxSpeed = FarMaxSpeed
    end

    -- 등가속도 운동 공식: s = v0*t + 0.5*a*t^2
    -- a = 2*(s - v0*t) / t^2
    obj.Speed = InitialSpeed
    obj.Acceleration = 2.0 * (obj.InitialDistance - InitialSpeed * DynamicReachTime) / (DynamicReachTime * DynamicReachTime)
    obj.DynamicMaxSpeed = DynamicMaxSpeed

    -- 음수 가속도 방지
    if obj.Acceleration < 0 then
        --print("[HomingProjectile] WARNING: Negative acceleration detected! Distance too short: " .. obj.InitialDistance)
        obj.Acceleration = 0
    end

    --print("[HomingProjectile] Initial distance: " .. obj.InitialDistance)
    --print("[HomingProjectile] Dynamic reach time: " .. DynamicReachTime .. "s, Max speed: " .. DynamicMaxSpeed)
    --print("[HomingProjectile] Calculated acceleration: " .. obj.Acceleration)
    --print("[HomingProjectile] Will reach target in " .. DynamicReachTime .. " seconds")
end

---
-- Target Enemy에 자신을 등록
---
function RegisterToTarget()
    local TargetScript = obj.TargetActor:GetScriptComponent()
    if not TargetScript then
        return
    end

    local TargetEnv = TargetScript:GetEnv()
    if TargetEnv["RegisterProjectile"] then
        TargetEnv["RegisterProjectile"](obj.UUID)
        --print("[HomingProjectile] Registered to target enemy")
    end
end

-- ==============================================================================
-- Tick Helper Functions
-- ==============================================================================

---
-- 타겟 상태 업데이트 (살아있는지, 죽었는지, 사라졌는지 확인)
---
function UpdateTargetStatus()
    -- 타겟이 이미 사라진 경우 업데이트 불필요
    if obj.TargetLost then
        return
    end

    -- TargetActor가 nil이 된 경우
    if not obj.TargetActor then
        obj.TargetLost = true
        --print("[HomingProjectile] Target lost (actor destroyed)! Locked to last position: " .. tostring(obj.TargetLocation))
        return
    end

    -- UUID가 일치하지 않는 경우
    if obj.TargetUUID and obj.TargetActor.UUID ~= obj.TargetUUID then
        obj.TargetLost = true
        --print("[HomingProjectile] Target lost (UUID mismatch)! Locked to last position: " .. tostring(obj.TargetLocation))
        return
    end

    -- Enemy가 죽었는지 체크
    local ScriptComp = obj.TargetActor:GetScriptComponent()
    if not ScriptComp then
        -- ScriptComponent가 없으면 정상적인 추적
        obj.TargetLocation = obj.TargetActor.Location
        return
    end

    local Env = ScriptComp:GetEnv()
    if Env["obj"] and Env["obj"].IsDead then
        -- Enemy가 죽었음! 즉시 TargetLost 플래그 설정
        obj.TargetLost = true
        obj.TargetLocation = obj.TargetActor.Location
        --print("[HomingProjectile] Target died! Switching to fast return mode. Distance: " .. (obj.TargetLocation - obj.Location):Length())
    else
        -- Target이 살아있음: 현재 위치 계속 업데이트
        obj.TargetLocation = obj.TargetActor.Location
    end
end

---
-- 속도 계산 및 적용
---
function CalculateAndApplySpeed(DistanceToTarget)
    if obj.TargetLost then
        -- 타겟이 사라진 경우: 빠르게 이동하되, 가까워지면 감속
        CalculateSpeedForLostTarget(DistanceToTarget)
    else
        -- 타겟이 살아있는 경우: 등가속도 운동 + 거리 기반 조절
        CalculateSpeedForActiveTarget(DistanceToTarget)
    end
end

---
-- 사라진 타겟에 대한 속도 계산
---
function CalculateSpeedForLostTarget(DistanceToTarget)
    local BaseSpeed = MaxSpeed
    local SlowdownDist = 5.0

    if DistanceToTarget < SlowdownDist then
        -- 목표 지점에 가까워질수록 감속 (오버슈팅 방지)
        local SlowdownRatio = DistanceToTarget / SlowdownDist
        local MinSlowdownSpeed = 25.0
        obj.Speed = MinSlowdownSpeed + (BaseSpeed - MinSlowdownSpeed) * SlowdownRatio
    else
        obj.Speed = BaseSpeed
    end
end

---
-- 살아있는 타겟에 대한 속도 계산 (등가속도 운동 + 거리 기반 조절)
---
function CalculateSpeedForActiveTarget(DistanceToTarget)
    -- 등가속도 운동: v = v0 + a*t
    obj.ElapsedTime = obj.ElapsedTime + (_G.GameData.DeltaTime or 0.016)
    local CalculatedSpeed = InitialSpeed + obj.Acceleration * obj.ElapsedTime

    -- 최대 속도 제한
    local CurrentMaxSpeed = obj.DynamicMaxSpeed or MaxSpeed
    CalculatedSpeed = math.min(CalculatedSpeed, CurrentMaxSpeed)
    CalculatedSpeed = math.max(CalculatedSpeed, InitialSpeed)

    -- 감속 없이 일정한 속도 유지 (충돌까지 빠르게)
    obj.Speed = CalculatedSpeed
end

---
-- 회전 계산 및 적용
---
function CalculateAndApplyRotation(ToTarget, DistanceToTarget, dt)
    -- 타겟이 매우 가까워도 계속 유도 (충돌까지)
    ToTarget:Normalize()

    -- 각도 기반 속도 제한 (제거됨)
    ApplyAngleBasedSpeedPenalty(ToTarget)

    -- 회전 속도 계산
    local RotationRate = CalculateRotationRate(ToTarget, DistanceToTarget)

    -- Lerp로 부드러운 회전
    obj.Direction = LerpVector(obj.Direction, ToTarget, RotationRate * dt)
    obj.Direction:Normalize()
end

---
-- 각도 기반 속도 제한 (각도가 크면 속도 감소)
---
function ApplyAngleBasedSpeedPenalty(ToTarget)
    -- 각도 페널티 제거 - 일정한 속도로 유도만 수행
    -- 이렇게 하면 타겟 근처에서 느려지지 않음
end

---
-- 회전 속도 계산
---
function CalculateRotationRate(ToTarget, DistanceToTarget)
    if obj.TargetLost then
        -- Target이 죽은 경우: 빠른 회전
        return RotationSpeed * 3.0
    end

    -- Target이 살아있는 경우: 거리 + 각도 기반 회전 속도
    local BaseRotationRate = RotationSpeed

    -- 거리 기반 회전 속도 증가
    if DistanceToTarget < CriticalDistance then
        local DistanceBoost = 1.0 + (1.0 - DistanceToTarget / CriticalDistance) * 1.5
        BaseRotationRate = RotationSpeed * DistanceBoost
    end

    -- 각도 기반 회전 속도 증가
    local DotProduct = obj.Direction.x * ToTarget.x + obj.Direction.y * ToTarget.y + obj.Direction.z * ToTarget.z
    local AngleFactor = (DotProduct + 1.0) / 2.0

    if AngleFactor < AngleThreshold then
        local AngleBoost = MinRotationBoost + (1.0 - AngleFactor / AngleThreshold) * (MaxRotationBoost - MinRotationBoost)
        BaseRotationRate = BaseRotationRate * AngleBoost
    end

    return BaseRotationRate
end

---
-- 이동 적용 및 최대 거리 체크
---
function ApplyMovement(dt)
    local Movement = obj.Direction * obj.Speed * dt
    obj.Location = obj.Location + Movement

    -- 이동 거리 누적
    obj.TraveledDistance = obj.TraveledDistance + Movement:Length()

    -- 최대 거리 도달 시 제거
    if obj.TraveledDistance >= obj.MaxDistance then
        --print("HomingProjectile reached max distance: " .. obj.TraveledDistance .. " >= " .. obj.MaxDistance)
        ReturnToPool()
    end
end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

function OnBeginOverlap(OtherActor)
    --print("[HomingProjectile] OnBeginOverlap called")

    if not obj.IsInitialized then
        --print("[HomingProjectile] Not initialized, ignoring overlap")
        return
    end

    -- 자신과의 충돌 무시
    if OtherActor.UUID == obj.UUID then
        --print("[HomingProjectile] Self-collision, ignoring")
        return
    end

    --print("[HomingProjectile] Hit: " .. OtherActor:GetName() .. " (UUID: " .. OtherActor.UUID .. ")")

    -- Target UUID 확인
    if obj.TargetUUID and OtherActor.UUID ~= obj.TargetUUID then
        --print("[HomingProjectile] Not our target (UUID mismatch), ignoring collision")
        return
    end

    -- Target과 충돌 처리
    HandleTargetCollision(OtherActor)
end

function OnEndOverlap(OtherActor)
    -- 투사체는 EndOverlap 처리 불필요
end

---
-- Target과의 충돌 처리
---
function HandleTargetCollision(OtherActor)
    local ScriptComp = OtherActor:GetScriptComponent()
    if not ScriptComp then
        --print("[HomingProjectile] No ScriptComponent on target")
        return
    end

    --print("[HomingProjectile] ScriptComponent found on target")
    local Env = ScriptComp:GetEnv()

    -- TakeDamage 함수가 있으면 Enemy로 판별
    if not Env["TakeDamage"] then
        --print("[HomingProjectile] No TakeDamage function on target")
        return
    end

    --print("[HomingProjectile] Hit our target!")

    -- Enemy가 살아있으면 데미지 처리
    if not Env["obj"] or not Env["obj"].IsDead then
        --print("[HomingProjectile] Dealing " .. obj.Damage .. " damage")
        Env["TakeDamage"](obj.Damage)
    else
        --print("[HomingProjectile] Target is already dead, skipping damage")
    end

    -- Enemy에서 자신을 등록 해제
    if Env["UnregisterProjectile"] then
        --print("[HomingProjectile] Unregistering from target enemy")
        Env["UnregisterProjectile"](obj.UUID)
    end

    -- 투사체 제거
    --print("[HomingProjectile] Returning to pool")
    ReturnToPool()
end

-- ==============================================================================
-- Pool Management
-- ==============================================================================

function ReturnToPool()
    --print("[HomingProjectile] Returning to pool: " .. obj.UUID)

    -- Enemy에서 자신을 등록 해제
    UnregisterFromTarget()

    -- 상태 초기화
    ResetState()

    -- ActorPool에 반납
    ActorPool:Return(Owner)
end

---
-- Target에서 자신을 등록 해제
---
function UnregisterFromTarget()
    if not (obj.TargetActor and obj.TargetUUID) then
        return
    end

    local TargetScript = obj.TargetActor:GetScriptComponent()
    if not TargetScript then
        return
    end

    local TargetEnv = TargetScript:GetEnv()
    if TargetEnv["UnregisterProjectile"] then
        --print("[HomingProjectile] Unregistering from target enemy before returning to pool")
        TargetEnv["UnregisterProjectile"](obj.UUID)
    end
end

---
-- 상태 초기화
---
function ResetState()
    obj.IsInitialized = false
    obj.TraveledDistance = 0.0
    obj.TargetActor = nil
    obj.TargetUUID = nil
    obj.TargetLocation = nil
    obj.TargetLost = false
    obj.ElapsedTime = 0.0
    obj.Acceleration = 0.0
    obj.InitialDistance = 0.0
    obj.DynamicMaxSpeed = MaxSpeed
    obj.Speed = InitialSpeed
end

-- ==============================================================================
-- Utility Functions
-- ==============================================================================

---
-- Vector Lerp (선형 보간)
---
function LerpVector(A, B, T)
    T = math.min(math.max(T, 0), 1)
    return Vector(
        A.x + (B.x - A.x) * T,
        A.y + (B.y - A.y) * T,
        A.z + (B.z - A.z) * T
    )
end
