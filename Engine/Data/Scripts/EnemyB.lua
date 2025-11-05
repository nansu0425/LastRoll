-- ==============================================================================
-- 직선 돌진형 Enemy 스크립트 (굴러가기 적용)
--
-- 1. 'Attacking' 상태를 제거. 이동 상태와 공격 로직을 완전히 분리.
-- 2. Tick 상단에서 매 프레임 공격 쿨다운을 확인.
-- 3. 쿨다운이 0이고 플레이어가 사거리(AttackDistance) 내에 있으면,
--    현재 'MoveState'와 관계없이 즉시 Attack() 호출.
-- 4. "Idle": 플레이어가 AttackDistance *밖에* 있을 때만 "Moving" 시작.
-- 5. "Idle" 중 플레이어와 너무 멀어지면(MaxDistanceBeforeRelocate),
--    플레이어 주변(RelocateRadius)으로 순간이동(재배치)함.
-- 6. "Moving": 목표를 향해 가속/이동. (이동 중 공격 가능)
-- 7. "Decelerating": 감속 후 "Idle"로 돌아감. (감속 중 공격 가능)
-- ==============================================================================

local Util = require("Data\\Scripts\\Util")
local ActorPool = require("Data/Scripts/ActorPool")

local AttackDelay = 2

---
-- 선형 보간 (Linear Interpolation)
-- @param a: 시작 값
-- @param b: 종료 값
-- @param t: 보간 계수 (0.0 ~ 1.0)
---
local function Lerp(a, b, t)
    return a + (b - a) * t
end

function Attack()
    if _G.PlayerData.bPlayerAlive then
        _G.PlayerData.PlayerEnv.TakeDamagePlayer(obj.Dmg)
    end
end

function BeginPlay()
    -- 기본 스탯
    obj.MaxHP = 65
    obj.HP = 65
    obj.Dmg = 5
    obj.CurrentAttackDelay = AttackDelay
    obj.IsDead = false
    obj.IsDying = false

    -- 풀링 및 넉백
    obj.TargetingProjectiles = {}
    obj.KnockbackDir = Vector(0,0,0)
    obj.KnockbackDis = 0

    -- ========== 직선 이동 로직용 변수 ==========
    obj.MoveState = "Idle"           -- 현재 상태: "Idle", "Moving", "Decelerating"
    obj.MaxSpeed = 30.0              -- 최대 이동 속도
    obj.CurrentSpeed = 0.0           -- 현재 이동 속도
    obj.Acceleration = 10.0          -- 가속도 (Lerp 계수)
    obj.Deceleration = 3.0           -- 감속도 (Lerp 계수)
    
    obj.TargetPosition = Vector(0,0,0)    -- 목표 지점 (플레이어의 마지막 위치)
    obj.MoveDirection = Vector(0,0,0) -- 이동 방향
    
    obj.MaxChaseDistance = 20.0      -- 플레이어를 지나친 후, 이 거리보다 멀어지면 감속
    obj.LastPlayerDistance = 0.0     -- 직전 프레임의 플레이어 거리 (지나쳤는지 확인용)
    
    obj.AttackDistance = 5.5         -- 공격 사거리
    obj.MinStopSpeed = 0.1           -- 감속 후 정지했다고 판단하는 속도
    
    obj.RollSpeedFactor = 55.0       -- 굴러가는 속도 계수 (Speed * Factor = Degrees/sec)
    
    -- ========== 재배치 로직용 변수 추가 ==========
    obj.MaxDistanceBeforeRelocate = 100.0 -- 이 거리보다 멀어지면 재배치
    obj.RelocateRadius = 70.0            -- 플레이어 중심 이 반경
    obj.GroundZ = 0.0                    -- 지면 높이
    obj.HasSetGroundZ = false            -- 지면 높이를 설정했는지 여부
    -- ================================================
    
    -- ========== 죽음 애니메이션 변수 ============
    obj.IsDying = false
    obj.DeathInitialFlySpeed = 45.0
    obj.DeathGravity = 98.0
    obj.DeathFlightVelocity = Vector(0, 0, 0)
    obj.DeathSpinSpeed = 720.0
    obj.DeathSpinAxis = Vector(1, 0, 0)
    -- ================================================

    obj.HitSound = GetAudioComponentByName("knight.wav")
    obj.DeathSound = GetAudioComponentByName("knight_death.wav")
    
    -- Overlap 델리게이트 바인딩
    local SphereComp = Owner:GetComponent("USphereComponent")
    if SphereComp then
        SphereComp:BindBeginOverlap(self, OnBeginOverlap)
        SphereComp:BindEndOverlap(self, OnEndOverlap)
    else
        ULog("[Enemy] WARNING: No SphereComponent found!")
    end
end

function Knockback(dt)
    local CurKnockbackDis = obj.KnockbackDis * 0.01
    if CurKnockbackDis > obj.KnockbackDis then
        CurKnockbackDis = obj.KnockbackDis
    end

    if CurKnockbackDis < 0.005 then
        obj.Location = obj.Location + obj.KnockbackDir * CurKnockbackDis
        obj.KnockbackDis = 0
    else 
        obj.Location = obj.Location + obj.KnockbackDir * CurKnockbackDis
        obj.KnockbackDis  = obj.KnockbackDis - CurKnockbackDis
    end
end


---
-- 투사체가 호출할 데미지 받기 함수
-- @param InDamage: 받을 데미지
---
function TakeDamage(InDamage)
    obj.HP = obj.HP - InDamage
    Util.MakeDamageText(InDamage, obj.Location, Vector4(0.8,0.8,0.8,1))

    local Dir = obj.Location - _G.PlayerData.PlayerPos
    Dir:Normalize()
    obj.KnockbackDir = Dir
    obj.KnockbackDis = 3
    
    obj.HitSound:Play()

    if obj.HP <= 0 then
        Die()
    end
end

---
-- 사망 처리
---
function Die()
    obj.IsDead = true
    obj.IsDying = true
    obj.MoveState = "Idle" -- 이동 정지
    obj.CurrentSpeed = 0
    
    obj.DeathSound:Play()
    
    local UpwardForce = Vector(0, 0, obj.DeathInitialFlySpeed)
    local OutwardForce = obj.KnockbackDir * (obj.DeathInitialFlySpeed * 0.5)
    obj.DeathFlightVelocity = UpwardForce + OutwardForce
    
    obj.DeathSpinSpeed = Random(360, 720)
    obj.DeathSpinAxis = Vector(Random(-1, 1), Random(-1, 1), Random(-1, 1))
    if obj.DeathSpinAxis:Length() < 0.01 then
        obj.DeathSpinAxis = Vector(1, 0, 0)
    else 
        obj.DeathSpinAxis:Normalize()    
    end
end

---
-- Homing Projectile이 이 Enemy를 타겟으로 설정했을 때 등록
-- @param ProjectileUUID: 등록할 Homing Projectile의 UUID
---
function RegisterProjectile(ProjectileUUID)
    table.insert(obj.TargetingProjectiles, ProjectileUUID)
end

---
-- Homing Projectile이 충돌하거나 사라질 때 등록 해제
-- @param ProjectileUUID: 해제할 Homing Projectile의 UUID
---
function UnregisterProjectile(ProjectileUUID)
    for i, UUID in ipairs(obj.TargetingProjectiles) do
        if UUID == ProjectileUUID then
            table.remove(obj.TargetingProjectiles, i)
            CheckCanReturnToPool()
            return
        end
    end
end

---
-- 액터 풀에 반환할 수 있는지 확인
---
function CheckCanReturnToPool()
    if not obj.IsDead or not obj.IsDying then
        return
    end

    if #obj.TargetingProjectiles > 0 then 
        return
    end
    
    -- 지면에서 충분히 멀리 떨어지면 반납 (Z축 기준)
    if obj.HasSetGroundZ and obj.Location.z < (obj.GroundZ - 200.0) then
        ReturnToPool()
    end
end

---
-- ActorPool에 반납
---
function ReturnToPool()
    -- 기존 상태 초기화
    obj.IsDead = false
    obj.HP = obj.MaxHP
    obj.CurrentAttackDelay = AttackDelay
    obj.TargetingProjectiles = {}
    obj.KnockbackDir = Vector(0,0,0)
    obj.KnockbackDis = 0

    -- ========== 이동 상태 변수 초기화 ==========
    obj.MoveState = "Idle"
    obj.CurrentSpeed = 0.0
    obj.TargetPosition = Vector(0,0,0)
    obj.MoveDirection = Vector(0,0,0)
    obj.LastPlayerDistance = 0.0  
    obj.HasSetGroundZ = false
    -- ========================================
    
    -- ========== 죽음 상태 변수 초기화 ==========
    obj.IsDying = false
    obj.DeathFlightVelocity = Vector(0, 0, 0)
    obj.DeathSpinAxis = Vector(1, 0, 0)
    obj.Rotation = Quaternion(0, 0, 0, 1) 
    -- ========================================

    _G.GameData.GMEnv.AddScore(1)
    _G.GameData.GMEnv.AddEXP(1)
    ActorPool:Return(Owner)
end

function Tick(dt)
    if _G.GameData.GameState == EGameState.End then
        ReturnToPool()
        return
    end
    if Util.IsActiveMode() == false then
        return
    end
    
    -- ========== 죽음 상태 처리 ==========
    if obj.IsDying then 
        obj.DeathFlightVelocity.z = obj.DeathFlightVelocity.z - obj.DeathGravity * dt
        obj.Location = obj.Location + obj.DeathFlightVelocity * dt
        
        local SpinAngleDelta = obj.DeathSpinSpeed * dt
        Owner:AxisRotation(obj.DeathSpinAxis, SpinAngleDelta)
        
        CheckCanReturnToPool()
        return
    end
    -- ========================================

    Knockback(dt)
    
    local HPPer = obj.HP / obj.MaxHP
    Util.RenderHPBar(obj.Location, Vector2(50, 15), HPPer)

    -- 처음 Tick이 돌 때 현재 Z 위치를 "지면"으로 저장
    if not obj.HasSetGroundZ then
        obj.GroundZ = obj.Location.z
        obj.HasSetGroundZ = true
    end

    -- 플레이어와의 현재 거리 계산 (Z축 무시)
    local PlayerPos = _G.PlayerData.PlayerPos
    local DirToPlayer = PlayerPos - obj.Location
    DirToPlayer.z = 0 
    local CurrentPlayerDis = DirToPlayer:Length()

    -- ==============================================================================
    -- 공격 로직 (상태와 분리)
    -- ==============================================================================
    if obj.CurrentAttackDelay > 0 then
        obj.CurrentAttackDelay = obj.CurrentAttackDelay - dt
    end
    
    -- 쿨다운이 0이고, 사거리 내에 있다면 (현재 이동 상태와 관계없이) 공격
    if obj.CurrentAttackDelay <= 0 and CurrentPlayerDis <= obj.AttackDistance then
        Attack()
        obj.CurrentAttackDelay = AttackDelay 
    end
    
    -- ==============================================================================
    -- 직선 이동 상태 기계 (State Machine)
    -- ==============================================================================

    -- 상태: Idle (새로운 목표 설정)
    if obj.MoveState == "Idle" then
    
        -- ========== 재배치 로직 ==========
        if obj.HasSetGroundZ and CurrentPlayerDis > obj.MaxDistanceBeforeRelocate then
            local RandomAngle = math.rad(Random(0, 360))
            local RandomDist = Random(obj.RelocateRadius, obj.RelocateRadius * 1.1) 
            
            local OffsetX = math.cos(RandomAngle) * RandomDist
            local OffsetY = math.sin(RandomAngle) * RandomDist
            
            local NewX = PlayerPos.x + OffsetX
            local NewY = PlayerPos.y + OffsetY
            
            obj.Location = Vector(NewX, NewY, obj.GroundZ)
            
            -- 재배치 후 거리/방향 재계산
            DirToPlayer = PlayerPos - obj.Location
            DirToPlayer.z = 0 
            CurrentPlayerDis = DirToPlayer:Length()
            obj.HP = obj.MaxHP

        end
        -- =================================
    
        if CurrentPlayerDis > obj.AttackDistance then
            -- 플레이어 위치를 새 목표로 설정
            obj.TargetPosition = PlayerPos
            obj.MoveDirection = DirToPlayer
            obj.MoveDirection:Normalize()
            obj.MoveState = "Moving"
            
            -- 돌진 시작 시점의 거리를 기록
            obj.LastPlayerDistance = CurrentPlayerDis 
        end

    -- 상태: Moving (목표를 향해 가속/이동)
    elseif obj.MoveState == "Moving" then
        -- 목표 속도를 향해 가속 (Lerp)
        obj.CurrentSpeed = Lerp(obj.CurrentSpeed, obj.MaxSpeed, dt * obj.Acceleration)
        obj.Location = obj.Location + obj.MoveDirection * obj.CurrentSpeed * dt
        
        -- 이동 방향(x, y)에 수직인 축(-y, x)을 기준으로 회전
        if obj.CurrentSpeed > 0.01 then
            local RollAxis = Vector(-obj.MoveDirection.y, obj.MoveDirection.x, 0)
            local RollAngle = obj.CurrentSpeed * obj.RollSpeedFactor * dt
            Owner:AxisRotation(RollAxis, RollAngle)
        end
        
        -- 조건: 플레이어를 지나쳐서 거리가 멀어짐
        if CurrentPlayerDis > obj.LastPlayerDistance and CurrentPlayerDis > obj.MaxChaseDistance then
            obj.MoveState = "Decelerating"
        end
        
        -- 마지막 프레임 거리 갱신
        obj.LastPlayerDistance = CurrentPlayerDis

    -- 상태: Decelerating (감속)
    elseif obj.MoveState == "Decelerating" then
        -- 0을 향해 감속 (Lerp)
        obj.CurrentSpeed = Lerp(obj.CurrentSpeed, 0.0, dt * obj.Deceleration)
        obj.Location = obj.Location + obj.MoveDirection * obj.CurrentSpeed * dt
        
        if obj.CurrentSpeed > 0.01 then
            local RollAxis = Vector(-obj.MoveDirection.y, obj.MoveDirection.x, 0)
            local RollAngle = obj.CurrentSpeed * obj.RollSpeedFactor * dt
            Owner:AxisRotation(RollAxis, RollAngle)
        end
        
        -- 조건: 거의 멈췄으면 Idle로 전환
        if obj.CurrentSpeed < obj.MinStopSpeed then
            obj.CurrentSpeed = 0.0
            obj.MoveState = "Idle" 
        end
    end
end

function EndPlay()

end

-- ==============================================================================
-- Overlap Event Functions
-- ==============================================================================

function OnBeginOverlap(OtherActor)
    
end

function OnEndOverlap(OtherActor)
    
end