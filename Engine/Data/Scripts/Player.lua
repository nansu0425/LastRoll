local Util = require("Data\\Scripts\\Util")
local ActorPool = require("Data/Scripts/ActorPool")

if _G.PlayerData == nil then
_G.PlayerData = {}
end

_G.PlayerData.PlayerPos = Vector(0,0,0)

-- Projectile 활성화/비활성화 플래그
local bLinearProjectileEnabled = true   -- Linear Projectile 활성화 여부
local bHomingProjectileEnabled = true   -- Homing Projectile 활성화 여부
local bOrbitProjectileEnabled = true    -- Orbit Projectile 활성화 여부

-- 발사 설정
local AttackCooldown = 0.3  -- 발사 주기 (초)
local LinearProjectileSpeed = 50.0  -- Linear Projectile 속도
local HomingProjectileSpeed = 30.0  -- Homing Projectile 속도
local ProjectileDamage = 10
local ProjectileRange = 50.0

-- 자동 타겟 설정
local AutoTargetCooldown = 1.0  -- 자동 타겟 발사 주기 (초)
local AutoTargetRange = 30.0  -- Detection Collider 반지름 (Homing Projectile 감지 범위)

-- Orbit Projectile 설정
local OrbitRadius = 10.0        -- 회전 반경
local OrbitSpeed = 160.0        -- 회전 속도 (degree/s)
local OrbitCount = 3           -- 회전하는 투사체 개수
local OrbitDamage = 5          -- 충돌 데미지

-- Enemy 탐지 리스트
local DetectedEnemies = {}

-- Orbit Projectile 관리
local OrbitProjectiles = {}



function BeginPlay()

    -- DetectionCollider의 Overlap 이벤트 바인딩
    local DetectionCollider = Owner:GetComponent("USphereComponent")
    if DetectionCollider then
        print("[Player] Binding overlap to Detection Collider")
        DetectionCollider:BindBeginOverlap(self, OnDetectionBeginOverlap)
        DetectionCollider:BindEndOverlap(self, OnDetectionEndOverlap)
    else
        print("[Player] WARNING: No Detection Collider found!")
    end

    -- Init 호출 (외부에서 호출되지 않는 경우를 위해)
    if not obj.Speed then
        Init()
    end
end

function Init()
 -- Initialize custom properties
    obj.OverlapCount = 0
    obj.Speed = 14
    obj.MaxHP = 100.0
    obj.HP = 100.0
    obj.Dmg = 10 -- 현재 안씀
    obj.AttackTimer = 0.0  -- 발사 타이머
    obj.AutoTargetTimer = 0.0  -- 자동 타겟 타이머
    obj.Location = Vector(0,0,1)

    AttackCooldown = 0.3  -- 발사 주기 (초)
    LinearProjectileSpeed = 50.0
    HomingProjectileSpeed = 30.0
    ProjectileDamage = 10
    ProjectileRange = 50.0

    AutoTargetCooldown = 1.0  -- 자동 타겟 발사 주기 (초)
    AutoTargetRange = 30.0  -- Detection Collider 반지름 (Homing Projectile 감지 범위)

    SetOrbitRadius(10.0)       -- 회전 반경
    SetOrbitSpeed(160)      -- 회전 속도 (degree/s)
    SetOrbitCount(3)          -- 회전하는 투사체 개수
    OrbitDamage = 5          -- 충돌 데미지


    _G.PlayerData.PlayerEnv =  Owner:GetScriptComponentByName("Player.lua"):GetEnv()
    _G.PlayerData.bPlayerAlive = true

    -- Orbit Projectile 생성 (활성화 상태인 경우만)
    if bOrbitProjectileEnabled then
        SpawnOrbitProjectiles()
    end

    TopCamera()
    print("[Player] Actor Init: " .. obj.UUID)

end


function LevelUp(CurLevel)
Util.MakeTrailText("LevelUp++", obj.Location + Vector(3,0,0), Vector4(0,1,0,1))
ProjectileDamage = ProjectileDamage + 2
AttackCooldown = AttackCooldown - 0.02
if CurLevel == 3 then
SetOrbitCount(4)
LinearProjectileSpeed = 55
elseif CurLevel == 5 then
SetOrbitCount(5)
SetOrbitSpeed(220)
SetOrbitRadius(12.0)
SetOrbitDamage(8)
LinearProjectileSpeed = 60
end
end

-- Called every frame
-- @param dt: Delta time in seconds
function Tick(dt)
    if Util.IsActiveMode() == false then
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
    --print("Actor ending: " .. obj.UUID)
    --print("Total overlaps detected: " .. obj.OverlapCount)
end

-- ==============================================================================
-- Detection Collider Overlap Event Functions
-- ==============================================================================

---
-- Detection Collider에 Enemy 진입
---
function OnDetectionBeginOverlap(OtherActor)
    --print("[Player] Detection overlap started: " .. OtherActor:GetName())

    -- Enemy인지 확인 (ScriptComponent의 TakeDamage 함수로 판별)
    local ScriptComp = OtherActor:GetScriptComponent()
    if ScriptComp then
        local Env = ScriptComp:GetEnv()
        if Env["TakeDamage"] then
            -- Enemy로 판별됨
            table.insert(DetectedEnemies, OtherActor)
            --print("[Player] Enemy detected! Total enemies: " .. #DetectedEnemies)
        end
    end
end

---
-- Detection Collider에서 Enemy 이탈
---
function OnDetectionEndOverlap(OtherActor)
    --print("[Player] Detection overlap ended: " .. OtherActor:GetName())

    -- 리스트에서 제거
    for i, Enemy in ipairs(DetectedEnemies) do
        if Enemy.UUID == OtherActor.UUID then
            table.remove(DetectedEnemies, i)
            --print("[Player] Enemy removed from detection. Remaining: " .. #DetectedEnemies)
            break
        end
    end
end


---
-- 투사체가 호출할 데미지 받기 함수
-- @param InDamage: 받을 데미지
---
function TakeDamagePlayer(InDamage)
    obj.HP = obj.HP - InDamage
    Util.MakeDamageText(InDamage, obj.Location, Vector4(1,0,0,1))
    
    HPPer = obj.HP / obj.MaxHP
    if HPPer < 0.8 then
        SetVignette(Vector(1, 0, 0), 1 - HPPer) 
    end
    -- 사망 처리
    if obj.HP <= 0 then
        Die()
    end
end

---
-- 사망 처리
---
function Die()
    _G.PlayerData.bPlayerAlive = false
    _G.PlayerData.PlayerEnv = nil

    -- Orbit Projectile 제거
    ClearOrbitProjectiles()

    ActorPool:Return(Owner)
    _G.GameData.GMEnv.PlayerDead()
end

function TopCamera()
TargetPos = obj.Location
GetCamera().Location = TargetPos + Vector(-5,0,25)
GetCamera().Rotation = Vector(0, 70, 0)
end

function Move(dt)
    MoveDir = Vector(0,0,0)
    if IsKeyDown(EKeyInput.W) then
        MoveDir.x = MoveDir.x + 1
        Owner:AxisRotation(Vector(0,1,0), 500 * dt)
    end
    if IsKeyDown(EKeyInput.A) then
        MoveDir.y = MoveDir.y - 1
        Owner:AxisRotation(Vector(1,0,0), 500 * dt)
    end
    if IsKeyDown(EKeyInput.S) then
        MoveDir.x = MoveDir.x - 1
        Owner:AxisRotation(Vector(0,1,0), -500 * dt)
    end
    if IsKeyDown(EKeyInput.D) then
        MoveDir.y = MoveDir.y + 1
        Owner:AxisRotation(Vector(1,0,0), -500 * dt)
    end

    MoveDir:Normalize()
    obj.Location = obj.Location + MoveDir * obj.Speed * dt
    _G.PlayerData.PlayerPos = obj.Location

    --모든 Enemy 돌면서 겹치면 밀리도록 처리

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
    -- Linear Projectile이 비활성화된 경우 발사하지 않음
    if not bLinearProjectileEnabled then
        return
    end

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
                Env["Setup"](ShootDirection, LinearProjectileSpeed, ProjectileDamage, ProjectileRange)
            else
                --print("[Player] ERROR: Setup function not found in Projectile!")
            end
        else
            --print("[Player] ERROR: ScriptComponent not found!")
        end
    else
        --print("[Player] Failed to get projectile from pool")
    end
end

-- ==============================================================================
-- Auto Target Functions
-- ==============================================================================

---
-- 자동 타겟 발사 처리
---
function HandleAutoTarget(dt)
    -- Homing Projectile이 비활성화된 경우 처리하지 않음
    if not bHomingProjectileEnabled then
        return
    end

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
                --print("[Player] Skipping dead enemy: " .. Enemy:GetName())
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
                Env["Setup"](TargetActor, HomingProjectileSpeed, ProjectileDamage, ProjectileRange)
                --print("[Player] Homing projectile launched at target: " .. TargetActor:GetName())
            else
                --print("[Player] ERROR: Setup function not found in HomingProjectile!")
            end
        else
            --print("[Player] ERROR: ScriptComponent not found!")
        end
    else
        --print("[Player] Failed to get homing projectile from pool")
    end
end

-- ==============================================================================
-- Orbit Projectile Functions
-- ==============================================================================

---
-- Orbit Projectile 생성
---
function SpawnOrbitProjectiles()
    -- Orbit Projectile이 비활성화된 경우 생성하지 않음
    if not bOrbitProjectileEnabled then
        print("[Player] Orbit Projectile is disabled. Not spawning.")
        return
    end

    -- 기존 Orbit Projectile 제거
    ClearOrbitProjectiles()

    -- OrbitCount만큼 생성
    for i = 1, OrbitCount do
        local Projectile = ActorPool:Get("AOrbitProjectile")

        if Projectile then
            -- 초기 각도 균등 분배: 360도 / OrbitCount
            local InitialAngle = (360.0 / OrbitCount) * (i - 1)

            -- 투사체 ScriptComponent 가져오기
            local ProjScript = Projectile:GetScriptComponent()
            if ProjScript then
                -- Setup 함수 호출 (OrbitProjectile.lua의 Setup 함수)
                local Env = ProjScript:GetEnv()
                if Env["Setup"] then
                    Env["Setup"](Owner, InitialAngle, OrbitRadius, OrbitSpeed, OrbitDamage)
                    table.insert(OrbitProjectiles, Projectile)
                    print("[Player] Orbit projectile #" .. i .. " spawned at angle " .. InitialAngle)
                else
                    print("[Player] ERROR: Setup function not found in OrbitProjectile!")
                end
            else
                print("[Player] ERROR: ScriptComponent not found in OrbitProjectile!")
            end
        else
            print("[Player] Failed to get orbit projectile from pool")
        end
    end

    print("[Player] Total orbit projectiles spawned: " .. #OrbitProjectiles)
end

---
-- 모든 Orbit Projectile 제거
---
function ClearOrbitProjectiles()
    for _, Projectile in ipairs(OrbitProjectiles) do
        if Projectile then
            -- ActorPool에 반납
            ActorPool:Return(Projectile)
        end
    end

    -- 배열 초기화
    OrbitProjectiles = {}
    print("[Player] All orbit projectiles cleared")
end

---
-- Orbit Projectile 개수 변경 (동적 업데이트)
-- @param NewCount: 새로운 Orbit Projectile 개수
---
function SetOrbitCount(NewCount)
    OrbitCount = NewCount

    -- Orbit Projectile이 활성화된 경우에만 재생성
    if bOrbitProjectileEnabled then
        SpawnOrbitProjectiles()
    end
end

---
-- Orbit Projectile 반경 변경 (동적 업데이트)
-- @param NewRadius: 새로운 회전 반경
---
function SetOrbitRadius(NewRadius)
    OrbitRadius = NewRadius

    -- 기존 Orbit Projectile의 반경 업데이트
    for _, Projectile in ipairs(OrbitProjectiles) do
        if Projectile then
            local ProjScript = Projectile:GetScriptComponent()
            if ProjScript then
                local Env = ProjScript:GetEnv()
                if Env["obj"] then
                    Env["obj"].Radius = NewRadius
                end
            end
        end
    end
end

---
-- Orbit Projectile 회전 속도 변경 (동적 업데이트)
-- @param NewSpeed: 새로운 회전 속도 (degree/s)
---
function SetOrbitSpeed(NewSpeed)
    OrbitSpeed = NewSpeed

    -- 기존 Orbit Projectile의 속도 업데이트
    for _, Projectile in ipairs(OrbitProjectiles) do
        if Projectile then
            local ProjScript = Projectile:GetScriptComponent()
            if ProjScript then
                local Env = ProjScript:GetEnv()
                if Env["obj"] then
                    Env["obj"].RotationSpeed = NewSpeed
                end
            end
        end
    end
end

---
-- Orbit Projectile 데미지 변경 (동적 업데이트)
-- @param NewDamage: 새로운 충돌 데미지
---
function SetOrbitDamage(NewDamage)
    OrbitDamage = NewDamage

    -- 기존 Orbit Projectile의 데미지 업데이트
    for _, Projectile in ipairs(OrbitProjectiles) do
        if Projectile then
            local ProjScript = Projectile:GetScriptComponent()
            if ProjScript then
                local Env = ProjScript:GetEnv()
                if Env["obj"] then
                    Env["obj"].Damage = NewDamage
                end
            end
        end
    end
end

-- ==============================================================================
-- Projectile Enable/Disable Functions
-- ==============================================================================

---
-- Linear Projectile 활성화
---
function EnableLinearProjectile()
    bLinearProjectileEnabled = true
    print("[Player] Linear Projectile ENABLED")
end

---
-- Linear Projectile 비활성화
---
function DisableLinearProjectile()
    bLinearProjectileEnabled = false
    print("[Player] Linear Projectile DISABLED")
end

---
-- Homing Projectile 활성화
---
function EnableHomingProjectile()
    bHomingProjectileEnabled = true
    print("[Player] Homing Projectile ENABLED")
end

---
-- Homing Projectile 비활성화
---
function DisableHomingProjectile()
    bHomingProjectileEnabled = false
    print("[Player] Homing Projectile DISABLED")
end

---
-- Orbit Projectile 활성화
---
function EnableOrbitProjectile()
    if bOrbitProjectileEnabled then
        print("[Player] Orbit Projectile is already ENABLED")
        return
    end

    bOrbitProjectileEnabled = true
    SpawnOrbitProjectiles()
    print("[Player] Orbit Projectile ENABLED")
end

---
-- Orbit Projectile 비활성화
---
function DisableOrbitProjectile()
    if not bOrbitProjectileEnabled then
        print("[Player] Orbit Projectile is already DISABLED")
        return
    end

    bOrbitProjectileEnabled = false
    ClearOrbitProjectiles()
    print("[Player] Orbit Projectile DISABLED")
end

---
-- 모든 Projectile 활성화
---
function EnableAllProjectiles()
    EnableLinearProjectile()
    EnableHomingProjectile()
    EnableOrbitProjectile()
    print("[Player] All Projectiles ENABLED")
end

---
-- 모든 Projectile 비활성화
---
function DisableAllProjectiles()
    DisableLinearProjectile()
    DisableHomingProjectile()
    DisableOrbitProjectile()
    print("[Player] All Projectiles DISABLED")
end

---
-- Projectile 활성화 상태 출력
---
function PrintProjectileStatus()
    print("=== Projectile Status ===")
    print("Linear Projectile: " .. (bLinearProjectileEnabled and "ENABLED" or "DISABLED"))
    print("Homing Projectile: " .. (bHomingProjectileEnabled and "ENABLED" or "DISABLED"))
    print("Orbit Projectile: " .. (bOrbitProjectileEnabled and "ENABLED" or "DISABLED"))
    print("========================")
end