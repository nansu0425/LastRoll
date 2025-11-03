

local Util = require("Data\\Scripts\\Util")
if _G.GameData == nil then
_G.GameData = {}
end

local LoadingText = ""
local EndSeuenceText = ""
local EnemySpawner = nil


function PlayerDead()
ChangeGameState(EGameState.EndSequence)
end


function StartSequence()
LoadingText = "3"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "2"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "1"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "0"
EnemySpawner:GetEnv().InitSpawner()
ChangeGameState(EGameState.Playing)
_G.GameData.Score = 0
end

function EndSequence()
EndSeuenceText = "죽었습니다.\n Score" .._G.GameData.Score
coroutine.yield(WaitForSeconds(3.0))
ChangeGameState(EGameState.End)
end

function ChangeGameState(InGameState)
if InGameState == EGameState.Lobby then
    --모든 오브젝트 제거  
elseif InGameState == EGameState.Loading then
    StartCoroutine("StartSequence")

elseif InGameState == EGameState.Playing then
    --캐릭터 생성
    --레벨매니저 스타트 해줘야함
elseif InGameState == EGameState.EndSequence then
    StartCoroutine("EndSequence")

elseif InGameState == EGameState.End then
    --레벨매니저 스탑
else
    print("Unknown State")
end
_G.GameData.GameState = InGameState

end


function BeginPlay()
_G.GameData.GameState = EGameState.Lobby
local ScriptComp = Owner:GetScriptComponentByName("EnemySpawner.lua")
if ScriptComp ~= nil then
EnemySpawner = ScriptComp
end
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    local Rect = GetViewportRect()
    local ScreenCenter = Vector2(Rect.x + Rect.z * 0.5, Rect.y + Rect.w * 0.5)
if _G.GameData.GameState == EGameState.Lobby then
    DrawText("To Start Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))
    if IsKeyDown(EKeyInput.W) then
    ChangeGameState(EGameState.Loading)
    end

elseif _G.GameData.GameState == EGameState.Loading then
    DrawText(LoadingText, ScreenCenter, Vector2(200,200), 70, Vector4(0.5,1,1,1))

elseif _G.GameData.GameState == EGameState.Playing then
    -- 캐릭터 생성
elseif _G.GameData.GameState == EGameState.EndSequence then
    DrawText(EndSeuenceText, ScreenCenter, Vector2(700,300), 50, Vector4(0.5,1,1,1))

elseif _G.GameData.GameState == EGameState.End then
    DrawText("Restart Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))
    if IsKeyDown(EKeyInput.W) then
    ChangeGameState(EGameState.Loading)
    StartCoroutine("StartSequence")
    end
end

end

-- Called once when the Actor ends play
function EndPlay()
    -- PIE 종료 시 ActorPool 정리 (중요!)
    local ActorPool = require("Data/Scripts/ActorPool")
    ActorPool:Clear()
    print("GameManager: ActorPool cleared on EndPlay")
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



