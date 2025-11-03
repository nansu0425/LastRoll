-- ==============================================================================
-- HomingProjectile.lua
-- ==============================================================================
-- 유도 투사체 행동 스크립트
-- 타겟 Enemy를 추적하여 자동으로 방향을 조정
-- ActorPool로 관리되는 재사용 가능한 투사체
-- ==============================================================================

local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")

-- 투사체 기본 설정
local InitialSpeed = 10.0  -- 초기 속도 (천천히 시작)
local TargetReachTime = 0.5  -- 목표 도달 시간 (초) - 거리에 상관없이 이 시간 내에 도달
local DefaultDamage = 15
local DefaultMaxDistance = 100.0
local RotationSpeed = 5.0  -- 유도 강도 (높을수록 급격하게 회전)
local MaxSpeed = 100.0  -- 최대 속도 제한 (안전장치)

-- Called once when the Actor begins play
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

    print("HomingProjectile created: " .. obj.UUID)

    -- Overlap 델리게이트 바인딩 - SphereComponent만 찾아서 바인딩
    local SphereComp = Owner:GetComponent("USphereComponent")
    if SphereComp then
        print("[HomingProjectile] Binding overlap to SphereComponent")
        SphereComp:BindBeginOverlap(self, OnBeginOverlap)
        SphereComp:BindEndOverlap(self, OnEndOverlap)
    else
        print("[HomingProjectile] WARNING: No SphereComponent found!")
    end
end

---
-- 투사체 초기화 함수 (발사 시 호출)
-- @param InTargetActor: 추적할 타겟 Actor
-- @param InSpeed: 투사체 속도 (사용하지 않음 - 거리에 따라 동적 변경)
-- @param InDamage: 데미지 (선택)
-- @param InMaxDistance: 최대 이동 거리 (선택)
---
function Setup(InTargetActor, InSpeed, InDamage, InMaxDistance)
    obj.TargetActor = InTargetActor
    obj.TargetLost = false  -- 타겟이 사라지지 않은 상태로 시작
    obj.Damage = InDamage or DefaultDamage
    obj.MaxDistance = InMaxDistance or DefaultMaxDistance
    obj.TraveledDistance = 0.0
    obj.ElapsedTime = 0.0

    -- Target UUID와 초기 위치 저장 (절대 변경되지 않음!)
    if obj.TargetActor then
        obj.TargetUUID = obj.TargetActor.UUID  -- Target의 고유 UUID 저장
        obj.TargetLocation = obj.TargetActor.Location  -- 초기 타겟 위치

        -- 초기 거리 측정
        obj.InitialDistance = (obj.TargetLocation - obj.Location):Length()

        -- 매우 가까운 거리 체크 (즉시 충돌 처리)
        if obj.InitialDistance < 2.0 then
            print("[HomingProjectile] Target is very close (" .. obj.InitialDistance .. "), attempting immediate collision")

            -- Enemy와 즉시 충돌 시도
            local TargetScript = obj.TargetActor:GetScriptComponent()
            if TargetScript then
                local TargetEnv = TargetScript:GetEnv()
                if TargetEnv["TakeDamage"] and TargetEnv["RegisterProjectile"] then
                    -- Enemy에 등록하고 즉시 데미지 처리
                    TargetEnv["RegisterProjectile"](obj.UUID)

                    if not TargetEnv["obj"].IsDead then
                        TargetEnv["TakeDamage"](obj.Damage)
                        print("[HomingProjectile] Immediate damage dealt")
                    end

                    TargetEnv["UnregisterProjectile"](obj.UUID)

                    -- 즉시 제거
                    obj.IsInitialized = false
                    ActorPool:Return(Owner)
                    return
                end
            end
        end

        obj.Direction = obj.TargetActor.Location - obj.Location
        obj.Direction:Normalize()
        print("[HomingProjectile] Target locked: " .. obj.TargetUUID)

        -- 거리에 따른 도달 시간 및 최대 속도 동적 조절
        local DynamicReachTime = TargetReachTime
        local DynamicMaxSpeed = MaxSpeed

        if obj.InitialDistance < 5.0 then
            -- 매우 가까운 거리 (< 5 units): 천천히 도달 (1.0초)
            DynamicReachTime = 1.0
            DynamicMaxSpeed = 30.0  -- 최대 속도 제한
        elseif obj.InitialDistance < 10.0 then
            -- 가까운 거리 (5~10 units): 적당한 속도 (0.75초)
            DynamicReachTime = 0.75
            DynamicMaxSpeed = 50.0
        else
            -- 먼 거리 (> 10 units): 빠르게 도달 (0.5초)
            DynamicReachTime = TargetReachTime
            DynamicMaxSpeed = MaxSpeed
        end

        -- 등가속도 운동 공식: s = v0*t + 0.5*a*t^2
        -- 목표: DynamicReachTime 초 안에 InitialDistance 거리 이동
        -- a = 2*(s - v0*t) / t^2
        obj.Speed = InitialSpeed
        obj.Acceleration = 2.0 * (obj.InitialDistance - InitialSpeed * DynamicReachTime) / (DynamicReachTime * DynamicReachTime)
        obj.DynamicMaxSpeed = DynamicMaxSpeed  -- 동적 최대 속도 저장

        -- 음수 가속도 방지 (너무 가까운 경우)
        if obj.Acceleration < 0 then
            print("[HomingProjectile] WARNING: Negative acceleration detected! Distance too short: " .. obj.InitialDistance)
            obj.Acceleration = 0  -- 등속 운동으로 변경
        end

        print("[HomingProjectile] Initial distance: " .. obj.InitialDistance)
        print("[HomingProjectile] Dynamic reach time: " .. DynamicReachTime .. "s, Max speed: " .. DynamicMaxSpeed)
        print("[HomingProjectile] Calculated acceleration: " .. obj.Acceleration)
        print("[HomingProjectile] Will reach target in " .. DynamicReachTime .. " seconds")

        -- Target Enemy에 자신을 등록 (Enemy가 죽어도 이 Projectile이 충돌할 때까지 대기)
        local TargetScript = obj.TargetActor:GetScriptComponent()
        if TargetScript then
            local TargetEnv = TargetScript:GetEnv()
            if TargetEnv["RegisterProjectile"] then
                TargetEnv["RegisterProjectile"](obj.UUID)
                print("[HomingProjectile] Registered to target enemy")
            end
        end
    end

    obj.IsInitialized = true
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    -- 게임 상태 체크
    if _G.GameData and _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    -- 초기화되지 않은 투사체는 이동하지 않음
    if not obj.IsInitialized then
        return
    end

    -- 타겟 UUID로 Target이 살아있는지 확인 (UUID가 일치하는지 체크)
    if not obj.TargetLost and obj.TargetActor and obj.TargetUUID then
        -- Target Actor가 여전히 유효하고 UUID가 일치하는지 확인
        if obj.TargetActor.UUID == obj.TargetUUID then
            -- UUID는 일치하지만, Enemy가 죽었는지 체크
            local ScriptComp = obj.TargetActor:GetScriptComponent()
            if ScriptComp then
                local Env = ScriptComp:GetEnv()
                if Env["obj"] and Env["obj"].IsDead then
                    -- Enemy가 죽었음! 즉시 TargetLost 플래그 설정
                    obj.TargetLost = true
                    obj.TargetLocation = obj.TargetActor.Location  -- 죽은 위치로 고정
                    print("[HomingProjectile] Target died! Switching to fast return mode. Distance: " .. (obj.TargetLocation - obj.Location):Length())
                else
                    -- Target이 살아있음: 현재 위치 계속 업데이트
                    obj.TargetLocation = obj.TargetActor.Location
                end
            else
                -- ScriptComponent가 없으면 정상적인 추적
                obj.TargetLocation = obj.TargetActor.Location
            end
        else
            -- UUID가 다름: Target이 변경되었거나 사라짐
            obj.TargetLost = true
            print("[HomingProjectile] Target lost (UUID mismatch)! Locked to last position: " .. tostring(obj.TargetLocation))
        end
    elseif not obj.TargetLost and obj.TargetUUID then
        -- TargetActor가 nil이 되었음: Target이 사라짐
        obj.TargetLost = true
        print("[HomingProjectile] Target lost (actor destroyed)! Locked to last position: " .. tostring(obj.TargetLocation))
    end

    -- TargetLocation을 향해 이동
    if obj.TargetLocation then
        -- 타겟 방향 계산
        local ToTarget = obj.TargetLocation - obj.Location
        local DistanceToTarget = ToTarget:Length()

        -- 마지막 타겟 위치에 도달했는지 체크 (타겟이 사라진 경우)
        if obj.TargetLost and DistanceToTarget < 1.0 then
            print("[HomingProjectile] Reached target location, returning to pool")
            ReturnToPool()
            return
        end

        -- 속도 조절
        if obj.TargetLost then
            -- 타겟이 사라진 경우: 빠르게 이동하되, 가까워지면 감속
            local BaseSpeed = MaxSpeed

            -- 목표 지점에 가까워질수록 감속 (오버슈팅 방지)
            local SlowdownDistance = 5.0  -- 이 거리부터 감속 시작
            if DistanceToTarget < SlowdownDistance then
                local SlowdownRatio = DistanceToTarget / SlowdownDistance
                -- 최소 속도를 40.0으로 높임 (더 빠르게 돌아오도록)
                local MinSlowdownSpeed = 40.0
                obj.Speed = MinSlowdownSpeed + (BaseSpeed - MinSlowdownSpeed) * SlowdownRatio
            else
                obj.Speed = BaseSpeed
            end
        else
            -- 타겟이 살아있는 경우: 등가속도 운동 적용
            -- v = v0 + a*t (속도 = 초기속도 + 가속도 × 경과시간)
            obj.ElapsedTime = obj.ElapsedTime + dt
            local CalculatedSpeed = InitialSpeed + obj.Acceleration * obj.ElapsedTime

            -- 최대 속도 제한 (거리에 따른 동적 최대 속도 사용)
            local CurrentMaxSpeed = obj.DynamicMaxSpeed or MaxSpeed
            if CalculatedSpeed > CurrentMaxSpeed then
                CalculatedSpeed = CurrentMaxSpeed
            end

            -- 음수 속도 방지
            if CalculatedSpeed < InitialSpeed then
                CalculatedSpeed = InitialSpeed
            end

            -- 거리 기반 속도 조절: 3단계 시스템
            local CriticalDistance = 8.0   -- 급가속 시작 거리 (스치는 것을 방지)
            local SlowdownDistance = 1.5   -- 감속 시작 거리 (마지막 순간에만 감속)

            if DistanceToTarget < SlowdownDistance then
                -- 1단계: 매우 가까운 거리 - 감속 (충돌 직전)
                local SlowdownRatio = DistanceToTarget / SlowdownDistance
                local MinSlowdownSpeed = InitialSpeed * 0.5
                obj.Speed = MinSlowdownSpeed + (CalculatedSpeed - MinSlowdownSpeed) * SlowdownRatio
            elseif DistanceToTarget < CriticalDistance then
                -- 2단계: 중간 거리 (위험 구간) - 적당한 가속 (스치지 않도록)
                -- 가까워질수록 더 빠르게 (SlowdownDistance에 가까울수록 최대 속도)
                local BoostRatio = 1.0 - ((DistanceToTarget - SlowdownDistance) / (CriticalDistance - SlowdownDistance))
                local BoostSpeed = CurrentMaxSpeed * 1.3  -- 동적 최대 속도의 130%까지 가속
                obj.Speed = CalculatedSpeed + (BoostSpeed - CalculatedSpeed) * BoostRatio
            else
                -- 3단계: 멀리 있을 때 - 정상 가속
                obj.Speed = CalculatedSpeed
            end
        end

        -- 타겟이 매우 가까우면 유도 중지 (오버슈팅 방지)
        if DistanceToTarget > 0.5 then
            ToTarget:Normalize()

            -- 현재 방향과 타겟 방향의 내적으로 각도 계산
            local DotProduct = obj.Direction.x * ToTarget.x + obj.Direction.y * ToTarget.y + obj.Direction.z * ToTarget.z
            -- DotProduct: 1.0 = 같은 방향, 0.0 = 수직, -1.0 = 반대 방향

            -- 각도가 크면 (DotProduct가 작으면) 속도를 크게 제한
            local AngleFactor = (DotProduct + 1.0) / 2.0  -- 0.0 ~ 1.0 범위로 정규화
            -- AngleFactor: 1.0 = 정렬됨, 0.5 = 수직, 0.0 = 반대

            -- 각도가 클 때 속도 제한 (회전 우선 모드)
            if AngleFactor < 0.85 then  -- 약 30도 이상 차이 날 때
                -- 속도를 크게 줄여서 회전에 집중
                local AnglePenalty = AngleFactor / 0.85  -- 0.0 ~ 1.0
                obj.Speed = obj.Speed * (0.3 + AnglePenalty * 0.7)  -- 최소 30%로 감속
                print("[HomingProjectile] Angle correction: DotProduct=" .. DotProduct .. ", Speed reduced to " .. obj.Speed)
            end

            -- Lerp로 부드러운 회전
            local RotationRate
            if obj.TargetLost then
                -- Target이 죽은 경우: 빠른 회전
                RotationRate = RotationSpeed * 3.0
            else
                -- Target이 살아있는 경우: 거리 + 각도 기반 회전 속도
                local BaseRotationRate = RotationSpeed

                if DistanceToTarget < 8.0 then
                    -- 가까운 거리: 회전 속도 증가
                    local DistanceBoost = 1.0 + (1.0 - DistanceToTarget / 8.0) * 1.5
                    BaseRotationRate = RotationSpeed * DistanceBoost
                end

                -- 각도가 크면 회전 속도 더욱 증가
                if AngleFactor < 0.85 then
                    local AngleBoost = 2.0 + (1.0 - AngleFactor / 0.85) * 3.0  -- 2.0 ~ 5.0배
                    BaseRotationRate = BaseRotationRate * AngleBoost
                end

                RotationRate = BaseRotationRate
            end

            obj.Direction = LerpVector(obj.Direction, ToTarget, RotationRate * dt)
            obj.Direction:Normalize()
        end
    end
    -- TargetLocation이 없으면 마지막 방향으로 직진

    -- 이동
    local Movement = obj.Direction * obj.Speed * dt
    obj.Location = obj.Location + Movement

    -- 이동 거리 누적
    local MovementLength = Movement:Length()
    obj.TraveledDistance = obj.TraveledDistance + MovementLength

    -- 최대 거리 도달 시 제거
    if obj.TraveledDistance >= obj.MaxDistance then
        print("HomingProjectile reached max distance: " .. obj.TraveledDistance .. " >= " .. obj.MaxDistance)
        ReturnToPool()
    end
end

-- Called once when the Actor ends play
function EndPlay()
    print("HomingProjectile ending: " .. obj.UUID)
end

-- ==============================================================================
-- Helper Functions
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

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

-- Called when overlap starts with another Actor
-- @param OtherActor: The Actor that began overlapping with this one
function OnBeginOverlap(OtherActor)
    print("[HomingProjectile] OnBeginOverlap called")

    if not obj.IsInitialized then
        print("[HomingProjectile] Not initialized, ignoring overlap")
        return
    end

    -- 자신과의 충돌 무시
    if OtherActor.UUID == obj.UUID then
        print("[HomingProjectile] Self-collision, ignoring")
        return
    end

    print("[HomingProjectile] Hit: " .. OtherActor:GetName() .. " (UUID: " .. OtherActor.UUID .. ")")

    -- Target UUID 확인: 우리의 원래 Target과 일치하는지 체크
    if obj.TargetUUID and OtherActor.UUID ~= obj.TargetUUID then
        print("[HomingProjectile] Not our target (UUID mismatch), ignoring collision")
        return
    end

    -- Target과 충돌했음! Enemy인지 확인하고 데미지 처리
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        print("[HomingProjectile] ScriptComponent found on target")
        local Env = ScriptComp:GetEnv()

        -- TakeDamage 함수가 있으면 Enemy로 판별
        if Env["TakeDamage"] ~= nil then
            print("[HomingProjectile] Hit our target!")

            -- Enemy가 살아있으면 데미지 처리
            if not Env["obj"] or not Env["obj"].IsDead then
                print("[HomingProjectile] Dealing " .. obj.Damage .. " damage")
                Env["TakeDamage"](obj.Damage)
            else
                print("[HomingProjectile] Target is already dead, skipping damage")
            end

            -- Enemy에서 자신을 등록 해제 (Enemy가 더 이상 타겟팅되지 않으면 자동으로 삭제됨)
            if Env["UnregisterProjectile"] then
                print("[HomingProjectile] Unregistering from target enemy")
                Env["UnregisterProjectile"](obj.UUID)
            end

            -- 투사체도 제거
            print("[HomingProjectile] Returning to pool")
            ReturnToPool()
        else
            print("[HomingProjectile] No TakeDamage function on target")
        end
    else
        print("[HomingProjectile] No ScriptComponent on target")
    end
end

-- Called when overlap ends with another Actor
-- @param OtherActor: The Actor that stopped overlapping with this one
function OnEndOverlap(OtherActor)
    -- 투사체는 EndOverlap 처리 불필요
end

---
-- ActorPool에 투사체 반납
---
function ReturnToPool()
    print("[HomingProjectile] Returning to pool: " .. obj.UUID)

    -- Enemy에서 자신을 등록 해제 (충돌 전에 최대 거리 도달 등으로 사라지는 경우)
    if obj.TargetActor and obj.TargetUUID then
        local TargetScript = obj.TargetActor:GetScriptComponent()
        if TargetScript then
            local TargetEnv = TargetScript:GetEnv()
            if TargetEnv["UnregisterProjectile"] then
                print("[HomingProjectile] Unregistering from target enemy before returning to pool")
                TargetEnv["UnregisterProjectile"](obj.UUID)
            end
        end
    end

    -- 초기화 플래그 리셋
    obj.IsInitialized = false
    obj.TraveledDistance = 0.0
    obj.TargetActor = nil
    obj.TargetUUID = nil  -- Target UUID 초기화
    obj.TargetLocation = nil  -- Target 위치 초기화
    obj.TargetLost = false  -- 타겟 소실 플래그 초기화
    obj.ElapsedTime = 0.0  -- 경과 시간 초기화
    obj.Acceleration = 0.0  -- 가속도 초기화
    obj.InitialDistance = 0.0  -- 초기 거리 초기화
    obj.DynamicMaxSpeed = MaxSpeed  -- 동적 최대 속도 초기화
    obj.Speed = InitialSpeed  -- 속도 초기화

    -- ActorPool에 반납
    ActorPool:Return(Owner)
end
