local Util = require("Data\\Scripts\\Util")

if _G.PlayerData == nil then
_G.PlayerData = {}
end

_G.PlayerData.PlayerPos = Vector(0,0,0)



function BeginPlay()
    -- Initialize custom properties
    --obj.Velocity = Vector(10, 0, 0)
    --obj.Speed = 100.0
    obj.OverlapCount = 0
    obj.Speed = 9
    obj.HP = 100
    obj.Dmg = 10
    print("Actor started: " .. obj.UUID)
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    Move(dt)  
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






function TopCamera()
TargetPos = obj.Location
GetCamera().Location = TargetPos + Vector(-2,0,10)
GetCamera().Rotation = Vector(0,-70,0)
end

function Move(dt)

MoveDir = Vector(0,0,0)
if IsKeyDown(EKeyInput.W) then
MoveDir.x = MoveDir.x + 1
end
if IsKeyDown(EKeyInput.A) then
MoveDir.y = MoveDir.y - 1
end
if IsKeyDown(EKeyInput.S) then
MoveDir.x = MoveDir.x - 1
end
if IsKeyDown(EKeyInput.D) then
MoveDir.y = MoveDir.y + 1
end

MoveDir:Normalize()
obj.Location = obj.Location + MoveDir * obj.Speed * dt
 _G.PlayerData.PlayerPos = obj.Location
TopCamera()
end