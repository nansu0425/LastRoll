-- ==============================================================================
-- LinearProjectile.lua
-- ==============================================================================
-- 직선 운동 투사체 행동 스크립트
-- ActorPool로 관리되는 재사용 가능한 투사체
-- ==============================================================================

local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")

-- 투사체 기본 설정
local DefaultSpeed = 50.0
local DefaultDamage = 10
local DefaultMaxDistance = 50.0

-- Called once when the Actor begins play
function BeginPlay()
    obj.Speed = DefaultSpeed
    obj.Damage = DefaultDamage
    obj.MaxDistance = DefaultMaxDistance
    obj.TraveledDistance = 0.0
    obj.Direction = Vector(1, 0, 0)
    obj.IsInitialized = false

   -- print("LinearProjectile created: " .. obj.UUID)

    -- Overlap 델리게이트 바인딩 - SphereComponent만 찾아서 바인딩
    local SphereComp = Owner:GetComponent("USphereComponent")
    if SphereComp then
        --print("[LinearProjectile] Binding overlap to SphereComponent")
        SphereComp:BindBeginOverlap(self, OnBeginOverlap)
        SphereComp:BindEndOverlap(self, OnEndOverlap)
    else
        --print("[LinearProjectile] WARNING: No SphereComponent found!")
    end
end

---
-- 투사체 초기화 함수 (발사 시 호출)
-- @param InDirection: 발사 방향 (정규화된 벡터)
-- @param InSpeed: 투사체 속도 (선택)
-- @param InDamage: 데미지 (선택)
-- @param InMaxDistance: 최대 이동 거리 (선택)
---
function Setup(InDirection, InSpeed, InDamage, InMaxDistance)
    obj.Direction = InDirection
    obj.Direction:Normalize()

    obj.Speed = InSpeed or DefaultSpeed
    obj.Damage = InDamage or DefaultDamage
    obj.MaxDistance = InMaxDistance or DefaultMaxDistance
    obj.TraveledDistance = 0.0
    obj.IsInitialized = true
end

-- Called every frame
-- @param dt: Delta time in seconds
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

    -- 이동
    local Movement = obj.Direction * obj.Speed * dt
    obj.Location = obj.Location + Movement

    -- 이동 거리 누적
    local MovementLength = Movement:Length()
    obj.TraveledDistance = obj.TraveledDistance + MovementLength

    -- 최대 거리 도달 시 제거
    if obj.TraveledDistance >= obj.MaxDistance then
        --print("LinearProjectile reached max distance: " .. obj.TraveledDistance .. " >= " .. obj.MaxDistance)
        ReturnToPool()
    end
end

-- Called once when the Actor ends play
function EndPlay()
    --print("LinearProjectile ending: " .. obj.UUID)
end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

-- Called when overlap starts with another Actor
-- @param OtherActor: The Actor that began overlapping with this one
function OnBeginOverlap(OtherActor)
    --print("[LinearProjectile] OnBeginOverlap called")

    if not obj.IsInitialized then
        --print("[LinearProjectile] Not initialized, ignoring overlap")
        return
    end

    -- 자신과의 충돌 무시
    if OtherActor.UUID == obj.UUID then
       --print("[LinearProjectile] Self-collision, ignoring")
        return
    end

   -- print("[LinearProjectile] Hit: " .. OtherActor:GetName() .. " (UUID: " .. OtherActor.UUID .. ")")

    -- Enemy와 충돌 체크
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        --print("[LinearProjectile] ScriptComponent found on target")
        local Env = ScriptComp:GetEnv()

        -- TakeDamage 함수가 있으면 호출 (Enemy 판별)
        if Env["TakeDamage"] ~= nil then
            -- 죽은 Enemy는 무시 (Homing Projectile이 처리할 것)
            if Env["obj"] and Env["obj"].IsDead then
                --print("[LinearProjectile] Target is dead, ignoring collision")
                return
            end

            --print("[LinearProjectile] TakeDamage function found, dealing " .. obj.Damage .. " damage")
            Env["TakeDamage"](obj.Damage)
            --print("[LinearProjectile] Returning to pool")
            ReturnToPool()
        else
            --print("[LinearProjectile] No TakeDamage function on target")
        end
    else
        --print("[LinearProjectile] No ScriptComponent on target")
    end
end

-- Called when overlap ends with another Actor
-- @param OtherActor: The Actor that stopped overlapping with this one
function OnEndOverlap(OtherActor)
    -- 투사체는 EndOverlap 처리 불필요
end

-- ==============================================================================
-- Helper Functions
-- ==============================================================================

---
-- ActorPool에 투사체 반납
---
function ReturnToPool()
    --print("[LinearProjectile] Returning to pool: " .. obj.UUID)

    -- 초기화 플래그 리셋
    obj.IsInitialized = false
    obj.TraveledDistance = 0.0

    -- ActorPool에 반납
    ActorPool:Return(Owner)
end
