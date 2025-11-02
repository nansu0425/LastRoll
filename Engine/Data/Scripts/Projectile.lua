-- ==============================================================================
-- Projectile.lua
-- ==============================================================================
-- 투사체 행동 스크립트
-- ActorPool로 관리되는 재사용 가능한 투사체
-- ==============================================================================

local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")

-- 투사체 기본 설정
local DefaultSpeed = 30.0
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

    print("Projectile created: " .. obj.UUID)
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

    print("========== Projectile Setup ==========")
    print(string.format("  Direction: (%.4f, %.4f, %.4f)", obj.Direction.x, obj.Direction.y, obj.Direction.z))
    print("  Speed: " .. obj.Speed)
    print("  Damage: " .. obj.Damage)
    print("  MaxDistance: " .. obj.MaxDistance)
    print("  IsInitialized: " .. tostring(obj.IsInitialized))
    print("======================================")
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

    -- 이동
    local Movement = obj.Direction * obj.Speed * dt
    obj.Location = obj.Location + Movement

    -- 이동 거리 누적
    local MovementLength = Movement:Length()
    obj.TraveledDistance = obj.TraveledDistance + MovementLength

    -- 디버그: 5프레임마다 거리 출력 (너무 많은 로그 방지)
    if not obj.TickCount then
        obj.TickCount = 0
    end
    obj.TickCount = obj.TickCount + 1

    if obj.TickCount % 30 == 0 then  -- 대략 0.5초마다 (60fps 기준)
        print("Projectile " .. obj.UUID .. " - Traveled: " .. string.format("%.2f", obj.TraveledDistance) .. " / " .. obj.MaxDistance)
    end

    -- 최대 거리 도달 시 제거
    if obj.TraveledDistance >= obj.MaxDistance then
        print("Projectile reached max distance: " .. obj.TraveledDistance .. " >= " .. obj.MaxDistance)
        ReturnToPool()
    end
end

-- Called once when the Actor ends play
function EndPlay()
    print("Projectile ending: " .. obj.UUID)
end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

-- Called when overlap starts with another Actor
-- @param OtherActor: The Actor that began overlapping with this one
function OnBeginOverlap(OtherActor)
    if not obj.IsInitialized then
        return
    end

    -- 자신과의 충돌 무시
    if OtherActor.UUID == obj.UUID then
        return
    end

    print("Projectile hit: " .. OtherActor:GetName())

    -- Enemy와 충돌 체크
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        local Env = ScriptComp:GetEnv()

        -- TakeDamage 함수가 있으면 호출 (Enemy 판별)
        if Env["TakeDamage"] ~= nil then
            print("Dealing damage: " .. obj.Damage)
            Env["TakeDamage"](obj.Damage)
            ReturnToPool()
        end
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
    print("Projectile returned to pool: " .. obj.UUID)

    -- 초기화 플래그 리셋
    obj.IsInitialized = false
    obj.TraveledDistance = 0.0
    obj.TickCount = 0

    -- ActorPool에 반납
    ActorPool:Return(Owner)
end
