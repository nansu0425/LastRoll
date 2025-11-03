



local Util = {}

EGameState = {
	Lobby = 1,
    Loading = 2,
	Playing = 3,
    EndSequence = 4
	End = 5
}

function Util.Clamp(value, min, max)
    if value < min then return min end
    if value > max then return max end
    return value
end

function Util.Round(value)
    return math.floor(value + 0.5)
end

function Util.Test()
print("test")
end

function Util.RenderHPBar(WorldPos, Size, HPPer)
DrawHPBar(WorldToScreenPos(WorldPos + Vector(1,0,0)), Size, HPPer)
end

function Util.MakeDamageText(InDamage, InWorldPos)
 local DamageText = 
    {
        Damage = InDamage,
        WorldPos = InWorldPos + Vector(2,0,0),
        LifeTime = 1.0
    }
table.insert(_G.UIData.DamageTextList, DamageText)
end

--function Util.RenderHPBar(WorldPos, Size, HPPer)
--DrawHPBar(WorldToScreenPos(WorldPos), Size, HPPer)
--end

return Util
