



local Util = {}

EGameState = {
	Lobby = 1,
    Loading = 2,
	Playing = 3,
    EndSequence = 4,
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
DrawGaugeBar(WorldToScreenPos(WorldPos + Vector(1,0,0)), Size, HPPer, Vector4(0.2,0.2,0.2,1.0), Vector4(1.0, 0.2, 0.2, 1.0))
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

function Util.IsActiveMode()
    if _G.GameData.GameState == EGameState.Playing or _G.GameData.GameState == EGameState.EndSequence then
        return true
    end
    return false
end

function Util.Vt2ToRotZDegree(Vt2)
    -- atan2(y, x) 는 2D 벡터가 X축과 이루는 라디안 각도 반환
    local rad = math.atan2(Vt2.y, Vt2.x)
    -- 라디안을 도 단위로 변환
    local deg = math.deg(rad)

    return deg
end
return Util
