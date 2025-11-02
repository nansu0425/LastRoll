local Util = require("Data\\Scripts\\Util")
local ActorPool = require("Data/Scripts/ActorPool")

if _G.PlayerData == nil then
_G.PlayerData = {}
end

_G.PlayerData.PlayerPos = Vector(0,0,0)

-- 발사 설정
local AttackCooldown = 0.3  -- 발사 주기 (초)
local ProjectileSpeed = 30.0
local ProjectileDamage = 10
local ProjectileRange = 50.0



function BeginPlay()
    -- Initialize custom properties
    obj.OverlapCount = 0
    obj.Speed = 9
    obj.MaxHP = 100.0
    obj.HP = 100.0
    obj.Dmg = 10
    obj.AttackTimer = 0.0  -- 발사 타이머
    print("Actor started: " .. obj.UUID)
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    if _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    Move(dt)
    HandleAttack(dt)  -- 발사 로직

    HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(70, 20), HPPer)
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

-- ==============================================================================
-- Attack Functions
-- ==============================================================================

---
-- 발사 로직 처리
---
function HandleAttack(dt)
    -- 쿨타임 감소
    if obj.AttackTimer > 0 then
        obj.AttackTimer = obj.AttackTimer - dt
    end

    -- 쿨타임이 끝나면 자동으로 마우스 방향으로 발사
    if obj.AttackTimer <= 0 then
        ShootProjectile()
        obj.AttackTimer = AttackCooldown
    end
end

---
-- 투사체 발사
---
function ShootProjectile()
    -- 마우스 위치 가져오기
    local MouseScreenPos = GetMousePosition()
    -- 3D 레이캐스팅으로 마우스 커서의 월드 위치 계산
    local MouseWorldPos2D = ScreenToWorldPosition(MouseScreenPos, obj.Location.z)

    -- 플레이어 위치에서 마우스 월드 위치로의 방향 계산
    local ShootDirection = Vector(
        MouseWorldPos2D.x - obj.Location.x,
        MouseWorldPos2D.y - obj.Location.y,
        0
    )
    ShootDirection:Normalize()

    -- ActorPool에서 투사체 가져오기 (AProjectileActor 사용)
    local Projectile = ActorPool:Get("AProjectileActor")

    if Projectile then
        -- 투사체 위치 설정 (플레이어 위치에서 약간 앞)
        local SpawnPos = obj.Location + ShootDirection * 2.0
        Projectile.Location = SpawnPos

        -- 투사체 ScriptComponent 가져오기 (타입 안전한 메서드 사용)
        local ProjScript = Projectile:GetScriptComponent()
        if ProjScript then
            -- Setup 함수 호출 (Projectile.lua의 Setup 함수)
            local Env = ProjScript:GetEnv()
            if Env["Setup"] then
                Env["Setup"](ShootDirection, ProjectileSpeed, ProjectileDamage, ProjectileRange)
            else
                print("[Player] ERROR: Setup function not found in Projectile!")
            end
        else
            print("[Player] ERROR: ScriptComponent not found!")
        end
    else
        print("[Player] Failed to get projectile from pool")
    end
end