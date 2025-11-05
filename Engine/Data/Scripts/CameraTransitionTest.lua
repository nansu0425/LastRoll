-- ============================================================================
-- Camera Transition Test Functions
-- ============================================================================
-- Player.lua 또는 다른 Actor 스크립트에서 require로 불러와서 사용
-- ============================================================================

local CameraTransitionTest = {}

-- 테스트 1: 위쪽 시점으로 이동
function CameraTransitionTest.TransitionToTopView()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(0, 0, 2000)  -- 위에서 내려다봄
    targetPOV.Rotation = Quaternion(0.7071, 0, 0, 0.7071)  -- 90도 아래를 봄
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "SlowZoom")
    print("[CameraTest] 위쪽 시점으로 이동 중... (2초, SlowZoom)")
end

-- 테스트 2: 정면 시점으로 이동
function CameraTransitionTest.TransitionToFrontView()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(1000, 0, 500)  -- 정면 위치
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)  -- 정면
    targetPOV.FOV = 75.0

    PlayTransitionPreset(targetPOV, "SmoothPan")
    print("[CameraTest] 정면 시점으로 이동 중... (1.5초, SmoothPan)")
end

-- 테스트 3: 측면 시점으로 이동
function CameraTransitionTest.TransitionToSideView()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(0, 1500, 500)  -- 옆에서 봄
    targetPOV.Rotation = Quaternion(0, 0, -0.7071, 0.7071)  -- 90도 왼쪽으로 회전
    targetPOV.FOV = 60.0

    PlayTransitionPreset(targetPOV, "Cinematic")
    print("[CameraTest] 측면 시점으로 이동 중... (3초, Cinematic)")
end

-- 테스트 4: 바운스 효과로 이동
function CameraTransitionTest.TransitionWithBounce()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(500, 500, 300)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "Bounce")
    print("[CameraTest] 바운스 효과로 이동 중... (1초, Bounce)")
end

-- 테스트 5: 빠른 컷 (거의 즉시 이동)
function CameraTransitionTest.TransitionQuickCut()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(-500, -500, 800)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "QuickCut")
    print("[CameraTest] 빠른 컷으로 이동... (0.1초, QuickCut)")
end

-- 테스트 6: Player 위치로 이동 (Player를 따라가는 카메라)
function CameraTransitionTest.TransitionToPlayer()
    local player = FindActorByName("Player")
    if not player then
        print("[CameraTest] Error: Player not found")
        return
    end

    local playerPos = player:GetLocation()
    local targetPOV = ViewInfo()
    -- Player 뒤쪽에서 위를 보는 카메라
    targetPOV.Location = playerPos + Vector(-300, 0, 200)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "SmoothPan")
    print("[CameraTest] Player 위치로 카메라 이동...")
end

-- 테스트 7: Custom Bezier 곡선 사용
function CameraTransitionTest.TransitionWithCustomCurve()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(800, 800, 600)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 70.0

    -- EaseInOut 커브 사용 (커스텀 Duration)
    local curve = BezierEaseInOut()
    StartCameraTransition(targetPOV, 2.5, curve)
    print("[CameraTest] 커스텀 Bezier 곡선으로 이동... (2.5초, EaseInOut)")
end

-- 모든 Transition 중지
function CameraTransitionTest.StopAllTransitions()
    StopAllCameraTransitions()
    print("[CameraTest] 모든 카메라 Transition 중지됨")
end

return CameraTransitionTest
