-- ==============================================================================
-- ActorPoolTestActor.lua
-- ==============================================================================
-- 액터풀 테스트용 예제 코드 
-- ==============================================================================

local LifeSpan = 1.0

-- Called once when the Actor begins play
function BeginPlay()
    -- Initialize custom properties
    obj.Velocity = Vector(10, 0, 0)
    obj.Speed = 100.0
    obj.OverlapCount = 0

    print("Actor started: " .. obj.UUID)
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    LifeSpan = LifeSpan - dt
    if LifeSpan <= 0 then 
        print("End!")
        LifeSpan = 1.0
        local TestPool = require("Data/Scripts/ActorPool")
        TestPool:Return(Owner)
    end
end

-- Called once when the Actor ends play
function EndPlay()
    print("Actor ending: " .. obj.UUID)
    print("Total overlaps detected: " .. obj.OverlapCount)
end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

-- Called when overlap starts with another Actor
-- @param OtherActor: The Actor that began overlapping with this one
function OnBeginOverlap(OtherActor)
    obj.OverlapCount = obj.OverlapCount + 1

    print("========== Overlap Started ==========")
    print("  My Actor: " .. obj.UUID)
    print("  Other Actor: " .. OtherActor:GetName())
    print("  Other Location: " .. tostring(OtherActor.Location))
    print("  Total Overlaps: " .. obj.OverlapCount)

    -- Example: Stop moving when overlapping
    -- obj.Velocity = Vector(0, 0, 0)

    -- Example: Change location on overlap
    -- obj.Location = obj.Location + Vector(0, 0, 10)
end

-- Called when overlap ends with another Actor
-- @param OtherActor: The Actor that stopped overlapping with this one
function OnEndOverlap(OtherActor)
    print("========== Overlap Ended ==========")
    print("  My Actor: " .. obj.UUID)
    print("  Other Actor: " .. OtherActor:GetName())

    -- Example: Resume movement after overlap
    -- obj.Velocity = Vector(10, 0, 0)
end
