-- ============================================================================
-- Camera Transition Test - Player.lua에 추가할 코드 예시
-- ============================================================================
-- 아래 코드를 Player.lua의 적절한 위치에 추가하세요
-- ============================================================================

-- ===== Player.lua 상단에 추가 =====
local CameraTest = require("Data/Scripts/CameraTransitionTest")

-- ===== BeginPlay 또는 Tick 함수에서 키 입력 체크 =====
function Tick(dt)
    -- 기존 Tick 코드...

    -- ===== 카메라 Transition 테스트 키 바인딩 =====

    -- Number 1: 위쪽 시점
    if IsKeyPressed(EKeyInput.Num1) then
        CameraTest.TransitionToTopView()
    end

    -- Number 2: 정면 시점
    if IsKeyPressed(EKeyInput.Num2) then
        CameraTest.TransitionToFrontView()
    end

    -- Number 3: 측면 시점
    if IsKeyPressed(EKeyInput.Num3) then
        CameraTest.TransitionToSideView()
    end

    -- Number 4: 바운스 효과
    if IsKeyPressed(EKeyInput.Num4) then
        CameraTest.TransitionWithBounce()
    end

    -- Number 5: 빠른 컷
    if IsKeyPressed(EKeyInput.Num5) then
        CameraTest.TransitionQuickCut()
    end

    -- Number 6: Player 위치로
    if IsKeyPressed(EKeyInput.Num6) then
        CameraTest.TransitionToPlayer()
    end

    -- Number 7: 커스텀 Bezier 곡선
    if IsKeyPressed(EKeyInput.Num7) then
        CameraTest.TransitionWithCustomCurve()
    end

    -- Number 0: 모든 Transition 중지
    if IsKeyPressed(EKeyInput.Num0) then
        CameraTest.StopAllTransitions()
    end
end

-- ============================================================================
-- 키 바인딩 요약:
-- ============================================================================
-- Number 1: 위쪽 시점 (SlowZoom, 2초)
-- Number 2: 정면 시점 (SmoothPan, 1.5초)
-- Number 3: 측면 시점 (Cinematic, 3초)
-- Number 4: 바운스 효과 (Bounce, 1초)
-- Number 5: 빠른 컷 (QuickCut, 0.1초)
-- Number 6: Player 위치로 (SmoothPan, 1.5초)
-- Number 7: 커스텀 곡선 (EaseInOut, 2.5초)
-- Number 0: 모든 Transition 중지
-- ============================================================================

-- ============================================================================
-- 또는 직접 함수 호출 (BeginPlay에서 자동 실행 등)
-- ============================================================================
function BeginPlay()
    -- 게임 시작 시 자동으로 카메라 이동 (예시)
    -- CameraTest.TransitionToTopView()
end
