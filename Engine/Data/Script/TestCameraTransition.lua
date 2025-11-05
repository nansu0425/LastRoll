-- ============================================================================
-- Camera Transition Test Script
-- ============================================================================
-- PIE 모드에서 이 스크립트를 실행하면 카메라가 여러 지점으로 이동합니다.
-- ============================================================================

-- 테스트 지점 1: 위쪽 시점
function TransitionToTopView()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(0, 0, 2000)  -- 위에서 내려다봄
    targetPOV.Rotation = Quaternion(0.7071, 0, 0, 0.7071)  -- 90도 아래를 봄
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "SlowZoom")
    print("카메라를 위쪽 시점으로 이동 중... (2초)")
end

-- 테스트 지점 2: 정면 시점
function TransitionToFrontView()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(1000, 0, 500)  -- 정면 위치
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)  -- 정면
    targetPOV.FOV = 75.0

    PlayTransitionPreset(targetPOV, "SmoothPan")
    print("카메라를 정면 시점으로 이동 중... (1.5초)")
end

-- 테스트 지점 3: 측면 시점
function TransitionToSideView()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(0, 1500, 500)  -- 옆에서 봄
    targetPOV.Rotation = Quaternion(0, 0, -0.7071, 0.7071)  -- 90도 왼쪽으로 회전
    targetPOV.FOV = 60.0

    PlayTransitionPreset(targetPOV, "Cinematic")
    print("카메라를 측면 시점으로 이동 중... (3초)")
end

-- 테스트 지점 4: 바운스 효과
function TransitionWithBounce()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(500, 500, 300)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "Bounce")
    print("카메라를 바운스 효과로 이동 중... (1초)")
end

-- 특정 Actor로 이동
function TransitionToActor(actorName)
    local actor = FindActorByName(actorName)
    if not actor then
        print("Error: Actor '" .. actorName .. "' not found")
        return
    end

    local targetPOV = ViewInfo()
    -- Actor 위치에서 약간 떨어진 곳에서 봄
    targetPOV.Location = actor:GetLocation() + Vector(-300, 0, 200)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    PlayTransitionPreset(targetPOV, "SmoothPan")
    print("카메라를 Actor '" .. actorName .. "'로 이동 중...")
end

-- 모든 Transition 중지
function StopTransitions()
    StopAllCameraTransitions()
    print("모든 카메라 Transition 중지됨")
end

-- ============================================================================
-- 사용 방법:
-- ============================================================================
-- 1. PIE 모드로 게임을 실행
-- 2. Console에서 함수 호출:
--    > TransitionToTopView()
--    > TransitionToFrontView()
--    > TransitionToSideView()
--    > TransitionWithBounce()
--    > TransitionToActor("Player")
--    > StopTransitions()
-- ============================================================================

print("TestCameraTransition.lua loaded!")
print("Available functions:")
print("  - TransitionToTopView()")
print("  - TransitionToFrontView()")
print("  - TransitionToSideView()")
print("  - TransitionWithBounce()")
print("  - TransitionToActor(actorName)")
print("  - StopTransitions()")
