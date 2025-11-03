



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

function Util.Sign(x)
    if x > 0 then
        return 1
    elseif x < 0 then
        return -1
    else
        return 0
    end
end

function Util.Test()
print("test")
end

function Util.RenderHPBar(WorldPos, Size, HPPer)
DrawGaugeBar(WorldToScreenPos(WorldPos + Vector(1,0,0)), Size, HPPer, Vector4(0.2,0.2,0.2,1.0), Vector4(1.0, 0.2, 0.2, 1.0))
end

function Util.MakeDamageText(InDamage, InWorldPos, InColor)
 local DamageText = 
    {
        Damage = InDamage,
        WorldPos = InWorldPos + Vector(2,0,0),
        LifeTime = 1.0,
        Color = InColor
    }
table.insert(_G.UIData.DamageTextList, DamageText)
end

function Util.IsActiveMode()
    if _G.GameData.GameState == EGameState.Playing or _G.GameData.GameState == EGameState.EndSequence then
        return true
    end
    return false
end

function Util.GetForwardFromDegreeZ(Degree)
    local rad = math.rad(Degree)
    local x = math.cos(rad)
    local y = math.sin(rad)
    return Vector2(x, y)
end

function Util.Rotate2DDegree(VtA, VtB)
    -- 각 벡터의 각도(라디안)
    local angleA = math.atan2(VtA.y, VtA.x)
    local angleB = math.atan2(VtB.y, VtB.x)

    -- B - A 회전 차이 (라디안)
    local diff = angleB - angleA

    -- -π ~ π 범위로 정규화
    if diff > math.pi then
        diff = diff - 2 * math.pi
    elseif diff < -math.pi then
        diff = diff + 2 * math.pi
    end

    -- 도 단위로 변환
    local degree = math.deg(diff)
    return degree
end
return Util
