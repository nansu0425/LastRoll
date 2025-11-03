
local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")
if _G.GameData == nil then
_G.GameData = {}
end

local LoadingText = ""
local EndSeuenceText = ""
local EnemySpawner = nil
local LevelEXP = {2, 2, 2, 2}
local MaxLevel = 5
local BlueLight
local PinkLight
local YellowLight
local BlueLightOffset
local PinkLightOffset
local YellowLightOffset
local CurLightPos = Vector(0,0,0)

function LightMove(dt)
LightToPlayer = _G.PlayerData.PlayerPos - CurLightPos
LightToPlayer.z = 0
CurLightPos = CurLightPos + LightToPlayer * dt * 1.3
SetLightPos()
end

function SetLightPos()
BlueLight.Location = CurLightPos + BlueLightOffset
PinkLight.Location = CurLightPos + PinkLightOffset
YellowLight.Location = CurLightPos + YellowLightOffset
end

function PlayerDead()
ChangeGameState(EGameState.EndSequence)
end


function StartSequence()
_G.GameData.Score = 0
_G.GameData.EXP = 0
_G.GameData.Level = 1

CurLightPos = Vector(0,0,0)
SetLightPos()

SpawnedActor = ActorPool:Get("APlayer")
print("캐릭터 생성")
SpawnedActor:GetScriptComponentByName("Player.lua"):GetEnv().Init()

LoadingText = "3"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "2"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "1"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "0"
EnemySpawner:GetEnv().InitSpawner()
ChangeGameState(EGameState.Playing)
--캐릭터 생성 필요
end

function AddScore(Score)
_G.GameData.Score = _G.GameData.Score + Score
end

function AddEXP(EXP)
_G.GameData.EXP = _G.GameData.EXP + EXP
if _G.GameData.Level < MaxLevel then
   if _G.GameData.EXP >=  LevelEXP[_G.GameData.Level] then
       _G.GameData.EXP = _G.GameData.EXP - LevelEXP[_G.GameData.Level]
       _G.GameData.Level = _G.GameData.Level + 1
       end
   end
end

function GetLevelText()
 if _G.GameData.Level >= MaxLevel then
    return "Lv Max"
    else
    return "Lv "..tostring(_G.GameData.Level)
    end
end

function GetCurEXPPer()
    if _G.GameData.Level >= MaxLevel then
        return 1
    else
        return _G.GameData.EXP / LevelEXP[_G.GameData.Level]
    end
end

function EndingSequence()
EndSeuenceText = "죽었습니다.\n Score" .._G.GameData.Score
coroutine.yield(WaitForSeconds(1.5))
CurLightPos = Vector(10000,10000,10000)
SetLightPos()
coroutine.yield(WaitForSeconds(1.5))
ChangeGameState(EGameState.End)
end

function ChangeGameState(InGameState)
if InGameState == EGameState.Lobby then
elseif InGameState == EGameState.Loading then
    StartCoroutine("StartSequence")
elseif InGameState == EGameState.EndSequence then
    StartCoroutine("EndingSequence")
end
_G.GameData.GameState = InGameState
end


function BeginPlay()
ActorPool:Clear()
_G.GameData.ManagerActor = Owner
_G.GameData.GMEnv = Owner:GetScriptComponentByName("GameManager.lua"):GetEnv()
_G.GameData.GameState = EGameState.Lobby
local ScriptComp = Owner:GetScriptComponentByName("EnemySpawner.lua")
if ScriptComp ~= nil then
EnemySpawner = ScriptComp
end

BlueLight = FindActorByName("BlueLight")
PinkLight = FindActorByName("PinkLight")
YellowLight = FindActorByName("YellowLight")
BlueLightOffset = BlueLight.Location
PinkLightOffset = PinkLight.Location
YellowLightOffset = YellowLight.Location

print(BlueLightOffset)

end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    local Rect = GetViewportRect()
    local ScreenCenter = Vector2(Rect.x + Rect.z * 0.5, Rect.y + Rect.w * 0.5)
    local ViewportLTop = Vector2(Rect.x, Rect.y)

if _G.GameData.GameState == EGameState.Lobby then
    DrawText("To Start Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))

    if IsKeyDown(EKeyInput.W) then
        ChangeGameState(EGameState.Loading)
        end

elseif _G.GameData.GameState == EGameState.Loading then
    DrawText(LoadingText, ScreenCenter, Vector2(200,200), 70, Vector4(0.5,1,1,1))

elseif _G.GameData.GameState == EGameState.Playing then
    --Score
    DrawText("Score "..tostring(_G.GameData.Score), Vector2(100,100) + ViewportLTop, Vector2(400,100), 40, Vector4(0,1,0,1))
    --Level, EXP
    DrawText(GetLevelText(), Vector2(ScreenCenter.x - 430, ViewportLTop.y + 100), Vector2(200,70), 40, Vector4(0,1,0,1))
    DrawGaugeBar(Vector2(ScreenCenter.x, ViewportLTop.y + 100), Vector2(700, 40), GetCurEXPPer(), Vector4(0.2,0.2,0.2,1.0), Vector4(0.0, 1.0, 0.0, 1.0))

    --LightMove
    LightMove(dt)

elseif _G.GameData.GameState == EGameState.EndSequence then
    DrawText(EndSeuenceText, ScreenCenter, Vector2(700,300), 50, Vector4(0.5,1,1,1))
    --LightMove
    LightMove(dt)

elseif _G.GameData.GameState == EGameState.End then
    DrawText("Restart Press W", ScreenCenter, Vector2(500,100), 50, Vector4(0.5,1,1,1))
    if IsKeyDown(EKeyInput.W) then
        ChangeGameState(EGameState.Loading)
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



