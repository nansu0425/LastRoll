

local Util = require("Data\\Scripts\\Util")
if _G.GameData == nil then
_G.GameData = {}
end

local Time = 0

function StartSequence()
Time = 3
coroutine.yield(WaitForSeconds(1.0))
Time = 2
coroutine.yield(WaitForSeconds(1.0))
Time = 1
coroutine.yield(WaitForSeconds(1.0))
Time = 0
ChangeGameState(EGameState.Playing)
end


function ChangeGameState(InGameState)
if InGameState == EGameState.Lobby then
    --모든 오브젝트 제거  
elseif InGameState == EGameState.Loading then

elseif InGameState == EGameState.Playing then
    --캐릭터 생성
    --레벨매니저 스타트 해줘야함
elseif InGameState == EGameState.End then
    --레벨매니저 스탑
else
    print("Unknown State")
end
_G.GameData.GameState = InGameState

end


function BeginPlay()
_G.GameData.GameState = EGameState.Lobby
print(_G.GameData.GameState)
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)

if _G.GameData.GameState == EGameState.Lobby then
    local Rect = GetViewportRect()
    local ScreenCenter = Vector2(Rect.x + Rect.z * 0.5, Rect.y + Rect.w * 0.5)
    DrawText("To Start Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))
    if IsKeyDown(EKeyInput.W) then
    ChangeGameState(EGameState.Loading)
    StartCoroutine("StartSequence")
    end
elseif _G.GameData.GameState == EGameState.Loading then
local Rect = GetViewportRect()
local ScreenCenter = Vector2(Rect.x + Rect.z * 0.5, Rect.y + Rect.w * 0.5)
DrawText(tostring(Time), ScreenCenter, Vector2(200,200), 70, Vector4(0.5,1,1,1))
elseif _G.GameData.GameState == EGameState.Playing then
    -- 캐릭터 생성
elseif _G.GameData.GameState == EGameState.End then
    -- 레벨매니저 스탑
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



