-- ==============================================================================
-- EnemySpawner.lua
-- ==============================================================================

local TestPool = require("Data/Scripts/ActorPool")
local Util = require("Data\\Scripts\\Util")

-- ==============================================================================
-- 스포너 설정 
-- ==============================================================================

-- 스폰 주기
local SpawnInterval = 5.0

-- 스폰 반경
local SpawnRadius = 80.0

-- 스폰 최소 반경
local MinSpawnRadius = 70.0

local SpawnTimer = 0.0

function InitSpawner()
print("InitSpawner")

end

-- Called once when the Actor begins play
function BeginPlay()
    -- @todo 게임 매니저가 존재할 경우 Clear를 게임 매니저에서 처리한다.
    TestPool:Clear()
    
    SpawnTimer = 0.0
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
     if _G.GameData.GameState ~= EGameState.Playing then
            return
        end
    SpawnTimer = SpawnTimer + dt
    
    if SpawnTimer >= SpawnInterval then 
        SpawnFromPool() 
        
        SpawnTimer = SpawnTimer - SpawnInterval
    end
end

-- Called once when the Actor ends play
function EndPlay()
    print("Actor ending: " .. obj.UUID)
end

-- ==============================================================================
-- 스폰 함수
-- ==============================================================================

function SpawnFromPool()
    local PlayerPos = _G.PlayerData.PlayerPos
    if not PlayerPos then 
        print("EnemySpawner:SpawnFromPool - PlayerPos is not available.")
        return
    end

    local SpawnedActor = TestPool:Get("AEightBallEnemy")
    if not SpawnedActor then 
        print("EnemySpawner:SpawnFromPool - Failed to get Enemy from Pool.")
        return
    end
    
    local Angle = Random(0, math.pi * 2)
    
    local Distance = Random(MinSpawnRadius, SpawnRadius)
    
    local SpawnX = PlayerPos.x + math.cos(Angle) * Distance
    local SpawnY = PlayerPos.y + math.sin(Angle) * Distance
    local SpawnZ = PlayerPos.z
    
    SpawnedActor.Location = Vector(SpawnX, SpawnY, SpawnZ)
end