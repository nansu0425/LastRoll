-- ==============================================================================
-- Lua Script Template for FutureEngine
-- ==============================================================================
-- This template provides the basic lifecycle functions for a scripted Actor.
--
-- Available Lifecycle Functions:
--   - BeginPlay()                 : Called when the Actor starts
--   - Tick(dt)                    : Called every frame with delta time
--   - EndPlay()                   : Called when the Actor is destroyed
--   - OnBeginOverlap(OtherActor)  : Called when overlap starts with another Actor
--   - OnEndOverlap(OtherActor)    : Called when overlap ends with another Actor
--
-- The "obj" variable represents the owner Actor and provides:
--   - obj.UUID              : Unique identifier (read-only)
--   - obj.Location          : Position (FVector, read/write)
--   - obj.Rotation          : Rotation (FQuaternion, read/write)
--   - obj.Velocity          : Custom velocity (FVector, read/write, script-managed)
--   - obj:PrintLocation()   : Prints the current location to console
--
-- Global Functions:
--   - Vector(x, y, z)       : Create a 3D vector
--   - Quaternion(x,y,z,w)   : Create a quaternion
--   - GetDeltaTime()        : Get current frame delta time
--   - GetTime()             : Get total game time
--   - ULog(message)         : Log to engine console
--   - print(...)            : Print to console (supports vectors, tables, etc.)
--
-- Example Usage:
--   obj.Velocity = Vector(10, 0, 0)
--   obj.Location = obj.Location + obj.Velocity * dt
-- ==============================================================================

-- Called once when the Actor begins play

CameraDegreeZ = 0
CameraDegreeY = 0




function BeginPlay()
    -- Initialize custom properties
    --obj.Velocity = Vector(10, 0, 0)
    --obj.Speed = 100.0
    obj.OverlapCount = 0
    obj.Speed = 5
    print("Actor started: " .. obj.UUID)
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)

    -- Update location based on velocity
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
    Movement = GetCamForward() * MoveDir.x
    Movement = Movement + GetCameraRight() * MoveDir.y
    obj.Location = obj.Location + Movement * obj.Speed * dt

    MouseSensitive = 0.2
    MouseDelta = GetMouseDelta()
    CameraDegreeZ = CameraDegreeZ + MouseDelta.x * MouseSensitive
    CameraDegreeY = CameraDegreeY + MouseDelta.y * MouseSensitive
    CameraDegreeY = math.max(-20, math.min(CameraDegreeY, 20))
    ThirdCamera()

    --StopCoroutine Test Success
    --if obj.Location.x > 15 then
        --StopCoroutine("WaitLambdaTest")
        --end
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






function ThirdCamera()
Distance = 2
Height = 1

TargetPos = obj.Location
Radian = math.rad(CameraDegreeZ + 180)
X = math.cos(Radian)
Y = math.sin(Radian)

CamPos = Vector(X, Y, 0) * Distance
CamPos.z = Height

CamPos = CamPos + TargetPos
GetCamera().Location = CamPos
GetCamera().Rotation = Vector(0,-CameraDegreeY,CameraDegreeZ)
end

function GetCamForward()
Radian = math.rad(CameraDegreeZ)
print(CameraDegreeZ)
print(Vector(math.cos(Radian), math.sin(Radian),0))

return Vector(math.cos(Radian), math.sin(Radian),0)
end

function GetCameraRight()
Radian = math.rad(CameraDegreeZ + 90)
return Vector(math.cos(Radian), math.sin(Radian),0)
end