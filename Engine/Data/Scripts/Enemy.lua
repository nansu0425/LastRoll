
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
    print("Enemy died!")

    -- ActorPool에 반납 (재사용)
    ActorPool:Return(Owner)
end

function Tick(dt)
    if _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    obj.CurAttackDelay = obj.CurAttackDelay - dt

    -- HP 바 렌더링 추가
    local HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(50, 15), HPPer)

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




