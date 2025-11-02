-- ==============================================================================
-- ActorPoolTest.lua
-- ==============================================================================
-- 액터풀 테스트용 예제 코드
-- ==============================================================================

local TestPool = require("Data/Scripts/ActorPool")

-- Called once when the Actor begins play
function BeginPlay()
    TestPool:Clear()
    print("Actor started: " .. obj.UUID)
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    if IsKeyPressed(EKeyInput.W) then 
        SpawnFromPool()
    end
end

-- Called once when the Actor ends play
function EndPlay()
    print("Actor ending: " .. obj.UUID)
end

function SpawnFromPool()
    local SpawnedActor = TestPool:Get("ACubeActor")
    
    if SpawnedActor then
        local X = Random(-10, 10)
        local Y = Random(-10, 10)
        local Z = Random(-10, 10)
        SpawnedActor.Location = Vector(X, Y, Z)
    end
end