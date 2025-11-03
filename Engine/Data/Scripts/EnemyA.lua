local Util = require("Data\\Scripts\\Util")
local ActorPool = require("Data/Scripts/ActorPool")

AttackDis = 1
AttackDelay = 2

---
-- 선형 보간 (Linear Interpolation)
-- @param a: 시작 값
-- @param b: 종료 값
-- @param t: 보간 계수 (0.0 ~ 1.0)
---
local function Lerp(a, b, t)
    return a + (b - a) * t
end

function Attack()
    print("Attack")
end

function BeginPlay()
    -- obj.Speed = 5 -- 이 변수는 StepDistance로 대체됩니다.
    obj.MaxHP = 50
    obj.HP = 50
    obj.Dmg = 5
    obj.CurAttackDelay = AttackDelay
    obj.IsDead = false
    obj.TargetingProjectiles = {}

    -- ========== 체스 말 이동 로직용 변수 추가 ==========
    obj.StepDistance = 10.0          -- 한 번의 "홉(hop)"으로 이동할 거리
    obj.MoveState = "Idle"           -- 현재 상태: "Idle", "Lifting", "Moving", "Landing"
    obj.MoveInterval = 1.0           -- 이동 사이의 대기 시간 (초)
    obj.MoveTimer = obj.MoveInterval -- 다음 이동까지 남은 시간

    obj.LiftHeight = 5.0            -- "홉" 할 때 떠오르는 높이 (Z축)
    obj.MoveAnimDuration = 1.0       -- "홉" 애니메이션(상승-이동-하강)에 걸리는 총 시간
    obj.MoveAnimTimer = 0.0          -- "홉" 애니메이션 타이머

    obj.StartMovePos = nil           -- 이동 시작 위치
    obj.TargetMovePos = nil          -- 이동 목표 위치
    obj.GroundZ = 0.0                -- 지면의 Z 레벨 (Tick에서 자동으로 설정됨)
    obj.HasSetGroundZ = false        -- GroundZ를 설정했는지 여부
    -- ================================================

    -- Overlap 델리게이트 바인딩 - SphereComponent만 찾아서 바인딩
    local SphereComp = Owner:GetComponent("USphereComponent")
    if SphereComp then
        print("[Enemy] Binding overlap to SphereComponent")
        SphereComp:BindBeginOverlap(self, OnBeginOverlap)
        SphereComp:BindEndOverlap(self, OnEndOverlap)
    else
        print("[Enemy] WARNING: No SphereComponent found!")
    end
end

---
-- 투사체가 호출할 데미지 받기 함수
-- @param InDamage: 받을 데미지
---
function TakeDamage(InDamage)
    obj.HP = obj.HP - InDamage

    -- 데미지 텍스트 표시
    Util.MakeDamageText(InDamage, obj.Location)

    print("Enemy HP: " .. obj.HP .. " / " .. obj.MaxHP)

    -- 사망 처리
    if obj.HP <= 0 then
        Die()
    end
end

---
-- 사망 처리
---
function Die()
    print("[Enemy] Enemy died! Entering dead state, waiting for homing projectiles...")
    print("[Enemy] Targeting projectiles count: " .. #obj.TargetingProjectiles)

    -- 죽은 상태로 전환 (즉시 삭제하지 않음)
    obj.IsDead = true
    obj.Speed = 0  -- 움직임 정지 (혹시 모를 상황 대비)

    -- 타겟팅하는 Projectile이 없으면 즉시 삭제
    if #obj.TargetingProjectiles == 0 then
        print("[Enemy] No targeting projectiles, returning to pool immediately")
        ReturnToPool()
    end
end

---
-- Homing Projectile이 이 Enemy를 타겟으로 설정했을 때 등록
-- @param ProjectileUUID: 등록할 Projectile의 UUID
---
function RegisterProjectile(ProjectileUUID)
    table.insert(obj.TargetingProjectiles, ProjectileUUID)
    print("[Enemy] Projectile registered: " .. ProjectileUUID .. " (Total: " .. #obj.TargetingProjectiles .. ")")
end

---
-- Homing Projectile이 충돌하거나 사라질 때 등록 해제
-- @param ProjectileUUID: 해제할 Projectile의 UUID
---
function UnregisterProjectile(ProjectileUUID)
    for i, UUID in ipairs(obj.TargetingProjectiles) do
        if UUID == ProjectileUUID then
            table.remove(obj.TargetingProjectiles, i)
            print("[Enemy] Projectile unregistered: " .. ProjectileUUID .. " (Remaining: " .. #obj.TargetingProjectiles .. ")")

            -- 죽은 상태에서 모든 Projectile이 사라지면 ActorPool에 반납
            if obj.IsDead and #obj.TargetingProjectiles == 0 then
                print("[Enemy] All targeting projectiles removed, returning to pool")
                ReturnToPool()
            end
            return
        end
    end
end

---
-- ActorPool에 반납 (모든 Homing Projectile이 충돌했을 때 호출)
---
function ReturnToPool()
    print("[Enemy] Returning to pool: " .. obj.UUID)

    -- 기존 상태 초기화
    obj.IsDead = false
    obj.HP = obj.MaxHP
    -- obj.Speed = 5 -- 삭제됨
    obj.CurAttackDelay = AttackDelay
    obj.TargetingProjectiles = {}

    -- ========== 이동 상태 변수 초기화 ==========
    obj.MoveState = "Idle"
    obj.MoveTimer = obj.MoveInterval
    obj.HasSetGroundZ = false -- 다음에 스폰될 때 GroundZ 다시 읽도록 설정
    obj.StartMovePos = nil
    obj.TargetMovePos = nil
    -- ========================================

    -- ActorPool에 반납 (재사용)
    _G.GameData.GMEnv.AddScore(1)
    _G.GameData.GMEnv.AddEXP(1)
    ActorPool:Return(Owner)
end

function Tick(dt)
    if _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    -- HP 바 렌더링 (죽은 상태에서도 표시)
    local HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(50, 15), HPPer)

    -- 죽은 상태면 로직 중단
    if obj.IsDead then
        return
    end

    -- 처음 Tick이 돌 때 현재 Z 위치를 "지면"으로 저장
    if not obj.HasSetGroundZ then
        obj.GroundZ = obj.Location.z
        obj.HasSetGroundZ = true
    end

    -- ==============================================================================
    -- 체스 말 이동 상태 기계 (State Machine)
    -- ==============================================================================

    if obj.MoveState == "Idle" then
        obj.MoveTimer = obj.MoveTimer - dt
        local Dir = _G.PlayerData.PlayerPos - obj.Location
        local Dis = Dir:Length()

        if Dis <= AttackDis then
            obj.CurAttackDelay = obj.CurAttackDelay - dt
            if obj.CurAttackDelay < 0 then
                Attack()
                obj.CurAttackDelay = AttackDelay
            end
            obj.MoveTimer = obj.MoveInterval
        elseif obj.MoveTimer <= 0 then
            obj.MoveState = "Lifting"
            obj.MoveAnimTimer = 0.0
            obj.StartMovePos = obj.Location
            Dir:Normalize()
            local TargetXY = obj.Location + Dir * obj.StepDistance
            local MaxMoveDist = math.max(0, Dis - AttackDis)
            if (TargetXY - obj.Location):Length() > MaxMoveDist then
                 TargetXY = obj.Location + Dir * MaxMoveDist
            end
            obj.TargetMovePos = Vector(TargetXY.x, TargetXY.y, obj.GroundZ)
            obj.StartMovePos.z = obj.GroundZ
        end

    elseif obj.MoveState == "Lifting" then
        -- Lifting 상태: 위로 떠오르기 (애니메이션 시간의 30%)
        obj.MoveAnimTimer = obj.MoveAnimTimer + dt
        local AnimTime = obj.MoveAnimDuration * 0.3
        local Alpha = obj.MoveAnimTimer / AnimTime
        
        local newZ -- 새로 계산할 Z 값

        if Alpha >= 1.0 then
            newZ = obj.GroundZ + obj.LiftHeight -- 정확히 최고 높이로
            obj.MoveState = "Moving"
            obj.MoveAnimTimer = 0.0          -- 다음 상태를 위해 타이머 리셋
        else
            -- 부드러운 시작 (EaseOutQuad)
            local t = Alpha
            newZ = Lerp(obj.GroundZ, obj.GroundZ + obj.LiftHeight, -t * (t - 2))
        end
        
        obj.Location = Vector(obj.StartMovePos.x, obj.StartMovePos.y, newZ)

    elseif obj.MoveState == "Moving" then
        -- Moving 상태: 수평 이동 (애니메이션 시간의 40%)
        obj.MoveAnimTimer = obj.MoveAnimTimer + dt
        local AnimTime = obj.MoveAnimDuration * 0.4
        local Alpha = obj.MoveAnimTimer / AnimTime

        local newX -- 새로 계산할 X 값
        local newY -- 새로 계산할 Y 값

        if Alpha >= 1.0 then
            newX = obj.TargetMovePos.x -- 정확히 목표 X, Y로
            newY = obj.TargetMovePos.y
            obj.MoveState = "Landing"
            obj.MoveAnimTimer = 0.0 -- 다음 상태를 위해 타이머 리셋
        else
            -- X, Y 좌표를 선형 보간 (Lerp)
            newX = Lerp(obj.StartMovePos.x, obj.TargetMovePos.x, Alpha)
            newY = Lerp(obj.StartMovePos.y, obj.TargetMovePos.y, Alpha)
        end
        
        obj.Location = Vector(newX, newY, obj.GroundZ + obj.LiftHeight)
        
    elseif obj.MoveState == "Landing" then
        -- Landing 상태: 아래로 착지 (애니메이션 시간의 30%)
        obj.MoveAnimTimer = obj.MoveAnimTimer + dt
        local AnimTime = obj.MoveAnimDuration * 0.3
        local Alpha = obj.MoveAnimTimer / AnimTime

        if Alpha >= 1.0 then
            obj.Location = obj.TargetMovePos -- 정확히 목표 지점으로 (Z 포함)
            obj.MoveState = "Idle"
            obj.MoveTimer = obj.MoveInterval -- 다음 이동 타이머 시작
        else
            local t = Alpha
            local newZ = Lerp(obj.GroundZ + obj.LiftHeight, obj.GroundZ, t * t)
            
            obj.Location = Vector(obj.TargetMovePos.x, obj.TargetMovePos.y, newZ)
        end
    end 
end

-- Called once when the Actor ends play
function EndPlay()

end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

-- Called when overlap starts with another Actor
-- @param OtherActor: The Actor that began overlapping with this one
function OnBeginOverlap(OtherActor)
    print("[Enemy] OnBeginOverlap called")
    print("[Enemy] Overlapped with: " .. OtherActor:GetName() .. " (UUID: " .. OtherActor.UUID .. ")")
end

-- Called when overlap ends with another Actor
-- @param OtherActor: The Actor that stopped overlapping with this one
function OnEndOverlap(OtherActor)
    print("[Enemy] OnEndOverlap called")
    print("[Enemy] Overlap ended with: " .. OtherActor:GetName())
end