
local Util = require("Data\\Scripts\\Util")

AttackDis = 1
AttackDelay = 2

function Attack()
print("Attack")
end

function BeginPlay()
    obj.Speed = 5
    obj.HP = 20
    obj.Dmg = 5
    obj.CurAttackDelay = AttackDelay
end


function Tick(dt)
    obj.CurAttackDelay = obj.CurAttackDelay - dt

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
  
end

-- Called when overlap ends with another Actor
-- @param OtherActor: The Actor that stopped overlapping with this one
function OnEndOverlap(OtherActor)
   
end




