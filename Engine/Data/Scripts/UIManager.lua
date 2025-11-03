if _G.UIData == nil then
_G.UIData = {}
end

_G.UIData.DamageTextList = {}

local Util = require("Data\\Scripts\\Util")

function DamageTextUIUpdate(dt)
    for i = #_G.UIData.DamageTextList, 1, -1 do
        local Text = _G.UIData.DamageTextList[i]
        Text.LifeTime = Text.LifeTime - dt
        if Text.LifeTime <= 0 then
            table.remove(_G.UIData.DamageTextList, i)
        else
            DrawText(tostring(Text.Damage), WorldToScreenPos(Text.WorldPos), Vector2(80,30), 30, Vector4(Text.Color.x,Text.Color.y,Text.Color.z,Text.LifeTime))
           Text.WorldPos = Text.WorldPos + Vector(1 * dt,0 ,0)
        end
    end
end
function BeginPlay()

end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
DamageTextUIUpdate(dt)
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





