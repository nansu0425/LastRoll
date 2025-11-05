
local ActorPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")
local CameraTest = require("Data/Scripts/CameraTransitionTest")

if _G.GameData == nil then
_G.GameData = {}
end

local LoadingText = ""
local EndSeuenceText = ""
local EnemyASpawner = nil
local EnemyBSpawner = nil
local LevelEXP = {2, 5, 15, 30}
local MaxLevel = 5
local BlueLight
local PinkLight
local YellowLight
local BlueLightOffset
local PinkLightOffset
local YellowLightOffset
local CurLightPos = Vector(0,0,0)
local AudioComponent

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
SetTimeDilation(1.0)
ResetVignette()
_G.GameData.Score = 0
_G.GameData.EXP = 0
_G.GameData.Level = 1

CurLightPos = Vector(0,0,0)
SetLightPos()

SpawnedActor = ActorPool:Get("APlayer")
print("캐릭터 생성")

-- ===== 게임 시작 카메라 연출 설정 =====
-- Player Init 전에 Follow Player 모드 비활성화
local PlayerEnv = SpawnedActor:GetScriptComponentByName("Player.lua"):GetEnv()
PlayerEnv.bFollowPlayer = false  -- Follow Player 모드 비활성화

-- Player 초기화 (TopCamera() 호출하여 기본 카메라 설정)
PlayerEnv.Init()

-- ===== 게임 시작 카메라 트랜지션 준비 =====
-- 1. 시작 카메라 위치: 플레이어 뒤쪽 먼 곳에서 높은 곳
local playerPos = SpawnedActor.Location
GetCamera().Location = playerPos + Vector(-100, 0, 80)  -- 뒤쪽 멀리, 높은 곳
GetCamera().Rotation = Vector(0, 60, 0)  -- 아래를 내려다보는 각도

-- 2. 시작 POV 저장
local startPOV = ViewInfo()
startPOV.Location = GetCamera().Location
startPOV.Rotation = QuaternionFromEuler(GetCamera().Rotation)
startPOV.FOV = 90.0
print("[GameManager] Start Location: ", startPOV.Location.x, startPOV.Location.y, startPOV.Location.z)

-- 3. TopCamera()를 호출해서 목표 위치/회전을 얻음
PlayerEnv.TopCamera()  -- 카메라를 최종 TopCamera 위치로 설정
print("[GameManager] TopCamera() 호출 완료")

-- 4. 목표 POV 저장
local targetPOV = ViewInfo()
targetPOV.Location = GetCamera().Location
targetPOV.Rotation = QuaternionFromEuler(GetCamera().Rotation)
targetPOV.FOV = 90.0
print("[GameManager] Target Location: ", targetPOV.Location.x, targetPOV.Location.y, targetPOV.Location.z)

-- ===== 카메라 트랜지션 시작 (동시에 카운트다운 진행) =====
print("[GameManager] 카메라 트랜지션 & 카운트다운 시작")
local result = PlayTransitionPreset(startPOV, targetPOV, "Cinematic")
print("[GameManager] PlayTransitionPreset 반환값: ", result)

StartCameraFade(1.0, 0.0, 3.0, Vector(0, 0, 0))

-- 트랜지션 진행 중 카운트다운 표시 (총 3초)
LoadingText = "3"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "2"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = "1"
coroutine.yield(WaitForSeconds(1.0))
LoadingText = ""  -- 카운트다운 종료

-- 트랜지션 완료 & Follow Player 모드 활성화
PlayerEnv.bFollowPlayer = true
print("[GameManager] 트랜지션 완료 - Follow Player 모드 활성화")

EnemyASpawner:GetEnv().InitSpawner()
EnemyBSpawner:GetEnv().InitSpawner()
ChangeGameState(EGameState.Playing)
--캐릭터 생성 필요

--배경음악 재생
AudioComponent = GetAudioComponentByName("valiant.wav")
AudioComponent:Play()
end

function LevelUp()
if _G.PlayerData.bPlayerAlive then
_G.PlayerData.PlayerEnv.LevelUp(_G.GameData.Level)
end
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
       LevelUp()
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
coroutine.yield(WaitForSeconds(0.15))
CurLightPos = Vector(10000,10000,10000)
SetLightPos()
coroutine.yield(WaitForSeconds(0.15))
ChangeGameState(EGameState.End)
AudioComponent:Stop()
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
local SpawnerAComp = Owner:GetScriptComponentByName("EnemySpawner.lua")
local SpawnerBComp = Owner:GetScriptComponentByName("EnemySpawnerB.lua")
EnemyASpawner = SpawnerAComp
EnemyBSpawner = SpawnerBComp

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



