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

local TILE_SIDE_LENGTH = 300.0

-- Called once when the Actor begins play
function BeginPlay()
    -- Initialize custom properties
    obj.WrapDistance = 2.0 * TILE_SIDE_LENGTH
    
    obj.WrapThreshold = 1.0 * TILE_SIDE_LENGTH
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    if not _G.PlayerData or not _G.PlayerData.PlayerPos then 
        print("Tile:Tick - Player not found.")
        return
    end

    local PlayerPos = _G.PlayerData.PlayerPos
    local TilePos = obj.Location
    
    local DeltaX = PlayerPos.x - TilePos.x 
    local DeltaY = PlayerPos.y - TilePos.y
    
    local NewX = TilePos.x 
    local NewY = TilePos.y 
    
    if DeltaX > obj.WrapThreshold then 
        NewX = NewX + obj.WrapDistance
    elseif DeltaX < -obj.WrapThreshold then
        NewX = NewX - obj.WrapDistance
    end

    if DeltaY > obj.WrapThreshold then
        NewY = NewY + obj.WrapDistance
    elseif DeltaY < -obj.WrapThreshold then
        NewY = NewY - obj.WrapDistance
    end

    obj.Location = Vector(NewX, NewY, TilePos.z)
end

-- Called once when the Actor ends play
function EndPlay()
end