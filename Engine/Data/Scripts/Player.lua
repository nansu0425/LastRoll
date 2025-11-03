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

-- 자동 타겟 설정
local AutoTargetCooldown = 1.0  -- 자동 타겟 발사 주기 (초)
local AutoTargetRange = 20.0  -- Detection Collider 반지름과 동일

-- Enemy 탐지 리스트
local DetectedEnemies = {}



function BeginPlay()
    -- Initialize custom properties
    obj.OverlapCount = 0
    obj.Speed = 9
    obj.MaxHP = 100.0
    obj.HP = 100.0
    obj.Dmg = 10
    obj.AttackTimer = 0.0  -- 발사 타이머
    obj.AutoTargetTimer = 0.0  -- 자동 타겟 타이머
    print("[Player] Actor started: " .. obj.UUID)

    -- DetectionCollider의 Overlap 이벤트 바인딩
    local DetectionCollider = Owner:GetComponent("USphereComponent")
    if DetectionCollider then
        print("[Player] Binding overlap to Detection Collider")
        DetectionCollider:BindBeginOverlap(self, OnDetectionBeginOverlap)
        DetectionCollider:BindEndOverlap(self, OnDetectionEndOverlap)
    else
        print("[Player] WARNING: No Detection Collider found!")
    end
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    if _G.GameData.GameState ~= EGameState.Playing then
        return
    end

    Move(dt)
    HandleAttack(dt)  -- 마우스 방향 발사 로직
    HandleAutoTarget(dt)  -- 자동 타겟 발사 로직

    HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(70, 20), HPPer)
end

-- Called once when the Actor ends play
function EndPlay()
    print("Actor ending: " .. obj.UUID)
    print("Total overlaps detected: " .. obj.OverlapCount)
end

-- ==============================================================================
-- Detection Collider Overlap Event Functions
-- ==============================================================================

---
-- Detection Collider에 Enemy 진입
---
function OnDetectionBeginOverlap(OtherActor)
    print("[Player] Detection overlap started: " .. OtherActor:GetName())

    -- Enemy인지 확인 (ScriptComponent의 TakeDamage 함수로 판별)
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        local Env = ScriptComp:GetEnv()
        if Env["TakeDamage"] then
            -- Enemy로 판별됨
            table.insert(DetectedEnemies, OtherActor)
            print("[Player] Enemy detected! Total enemies: " .. #DetectedEnemies)
        end
    end
end

---
-- Detection Collider에서 Enemy 이탈
---
function OnDetectionEndOverlap(OtherActor)
    print("[Player] Detection overlap ended: " .. OtherActor:GetName())

    -- 리스트에서 제거
    for i, Enemy in ipairs(DetectedEnemies) do
        if Enemy.UUID == OtherActor.UUID then
            table.remove(DetectedEnemies, i)
            print("[Player] Enemy removed from detection. Remaining: " .. #DetectedEnemies)
            break
        end
    end
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
        --_G.GameData.GMEnv.PlayerDead()
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

    -- ActorPool에서 투사체 가져오기 (ALinearProjectile 사용)
    local Projectile = ActorPool:Get("ALinearProjectile")

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

-- ==============================================================================
-- Auto Target Functions
-- ==============================================================================

---
-- 자동 타겟 발사 처리
---
function HandleAutoTarget(dt)
    -- 쿨타임 감소
    if obj.AutoTargetTimer > 0 then
        obj.AutoTargetTimer = obj.AutoTargetTimer - dt
        return
    end

    -- 가장 가까운 Enemy 찾기
    local TargetEnemy = FindNearestEnemy()

    if TargetEnemy then
        ShootHomingProjectile(TargetEnemy)
        obj.AutoTargetTimer = AutoTargetCooldown
    end
end

---
-- 가장 가까운 Enemy 찾기 (살아있는 Enemy만)
---
function FindNearestEnemy()
    if #DetectedEnemies == 0 then
        return nil
    end

    local NearestEnemy = nil
    local MinDistance = math.huge

    for _, Enemy in ipairs(DetectedEnemies) do
        -- 죽은 Enemy는 새로운 타겟으로 선택하지 않음
        local ScriptComp = Enemy:GetScriptComponent()
        if ScriptComp then
            local Env = ScriptComp:GetEnv()
            if Env["obj"] and Env["obj"].IsDead then
                -- 죽은 Enemy 무시
                print("[Player] Skipping dead enemy: " .. Enemy:GetName())
                goto continue
            end
        end

        -- Enemy가 여전히 유효한지 확인 (거리 계산이 성공하면 유효)
        local Distance = (Enemy.Location - obj.Location):Length()

        if Distance < MinDistance then
            MinDistance = Distance
            NearestEnemy = Enemy
        end

        ::continue::
    end

    return NearestEnemy
end

---
-- 유도 투사체 발사
---
function ShootHomingProjectile(TargetActor)
    local Projectile = ActorPool:Get("AHomingProjectile")

    if Projectile then
        -- 투사체 위치 설정 (플레이어 위치)
        Projectile.Location = obj.Location

        -- 투사체 ScriptComponent 가져오기
        local ProjScript = Projectile:GetScriptComponent()
        if ProjScript then
            -- Setup 함수 호출 (HomingProjectile.lua의 Setup 함수)
            local Env = ProjScript:GetEnv()
            if Env["Setup"] then
                Env["Setup"](TargetActor, ProjectileSpeed, ProjectileDamage, ProjectileRange)
                print("[Player] Homing projectile launched at target: " .. TargetActor:GetName())
            else
                print("[Player] ERROR: Setup function not found in HomingProjectile!")
            end
        else
            print("[Player] ERROR: ScriptComponent not found!")
        end
    else
        print("[Player] Failed to get homing projectile from pool")
    end
end