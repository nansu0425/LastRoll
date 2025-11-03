
local Util = require("Data\\Scripts\\Util")
local ActorPool = require("Data/Scripts/ActorPool")

AttackDis = 1
AttackDelay = 2

function Attack()
    print("Attack")
end

function BeginPlay()
    obj.Speed = 5
    obj.MaxHP = 20  -- MaxHP 추가
    obj.HP = 20
    obj.Dmg = 5
    obj.CurAttackDelay = AttackDelay
    obj.IsDead = false  -- 죽은 상태 플래그
    obj.TargetingProjectiles = {}  -- 이 Enemy를 타겟으로 하는 Homing Projectile UUID 리스트

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
    obj.Speed = 0  -- 움직임 정지

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

    -- 상태 초기화
    obj.IsDead = false
    obj.HP = obj.MaxHP
    obj.Speed = 5
    obj.CurAttackDelay = AttackDelay
    obj.TargetingProjectiles = {}  -- 타겟팅 리스트 초기화

    -- ActorPool에 반납
    ActorPool:Return(Owner)
end

function Tick(dt)
    if _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    -- HP 바 렌더링 (죽은 상태에서도 표시)
    local HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(50, 15), HPPer)

    -- 죽은 상태면 움직이지 않음 (Homing Projectile이 올 때까지 대기)
    if obj.IsDead then
        return
    end

    obj.CurAttackDelay = obj.CurAttackDelay - dt

    -- 플레이어 추적 로직
    Dir = _G.PlayerData.PlayerPos - obj.Location
    Dis = Dir:Length()
    if Dis > AttackDis then
        Dir:Normalize()
        obj.Location = obj.Location + Dir * obj.Speed * dt
    else
        if obj.CurAttackDelay < 0 then
            Attack()
            obj.CurAttackDelay = AttackDelay
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




