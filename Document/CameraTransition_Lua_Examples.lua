-- ============================================================================
-- Camera Transition System - Lua API Usage Examples
-- ============================================================================
--
-- This file demonstrates how to use the Camera Transition system from Lua.
-- The system allows smooth camera transitions with Bezier curve timing control.
--
-- Prerequisites:
-- - Must be in PIE (Play In Editor) or Game mode
-- - PlayerCameraManager must exist in the world
-- ============================================================================

-- ============================================================================
-- Example 1: Simple transition using preset
-- ============================================================================
function Example1_SimpleTransitionWithPreset()
    -- Create target view info
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(1000, 2000, 500)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    -- Play transition using "SlowZoom" preset (2s, EaseInOut)
    local transition = PlayTransitionPreset(targetPOV, "SlowZoom")

    if transition then
        print("Camera transition started with SlowZoom preset")
    end
end

-- ============================================================================
-- Example 2: Custom transition with custom Bezier curve
-- ============================================================================
function Example2_CustomTransition()
    -- Create target view info
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(500, 500, 300)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 75.0

    -- Use custom Bezier timing curve (EaseInOut)
    local timingCurve = BezierEaseInOut()

    -- Start transition with 3 second duration
    local transition = StartCameraTransition(targetPOV, 3.0, timingCurve)

    if transition then
        print("Custom camera transition started (3s, EaseInOut)")
    end
end

-- ============================================================================
-- Example 3: Using different Bezier curve presets
-- ============================================================================
function Example3_DifferentCurvePresets()
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(0, 0, 1000)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    -- Available Bezier curve presets:
    -- 1. BezierLinear()    - Linear interpolation (no easing)
    -- 2. BezierEaseIn()    - Slow start
    -- 3. BezierEaseOut()   - Slow end
    -- 4. BezierEaseInOut() - Slow start and end (S-curve)
    -- 5. BezierBounce()    - Bouncy effect

    local bounceCurve = BezierBounce()
    StartCameraTransition(targetPOV, 2.0, bounceCurve)
    print("Bouncy camera transition started!")
end

-- ============================================================================
-- Example 4: Checking transition progress
-- ============================================================================
function Example4_MonitorProgress()
    -- Start a transition
    local targetPOV = ViewInfo()
    targetPOV.Location = Vector(1000, 1000, 1000)
    targetPOV.Rotation = Quaternion(0, 0, 0, 1)
    targetPOV.FOV = 90.0

    local transition = PlayTransitionPreset(targetPOV, "Cinematic")

    if transition then
        -- Check if transition is active
        if transition:IsTransitioning() then
            -- Get current progress [0.0 - 1.0]
            local progress = transition:GetProgress()
            print("Transition progress: " .. (progress * 100) .. "%")
        end
    end
end

-- ============================================================================
-- Example 5: Stopping transitions
-- ============================================================================
function Example5_StopTransition()
    -- Stop a specific transition
    local transition = GetLastTransition() -- This would need to be stored from previous call
    if transition then
        transition:StopTransition()
        print("Transition stopped")
    end

    -- Or stop all transitions at once
    StopAllCameraTransitions()
    print("All camera transitions stopped")
end

-- ============================================================================
-- Example 6: Transition to actor's location
-- ============================================================================
function Example6_TransitionToActor()
    -- Find an actor by name
    local targetActor = FindActorByName("TargetActor")

    if targetActor then
        -- Create view info from actor's transform
        local targetPOV = ViewInfo()
        targetPOV.Location = targetActor:GetLocation() + Vector(0, 0, 500) -- Offset up by 500
        targetPOV.Rotation = targetActor:GetRotation()
        targetPOV.FOV = 90.0

        -- Smooth transition to actor's position
        PlayTransitionPreset(targetPOV, "SmoothPan")
        print("Transitioning camera to " .. targetActor:GetName())
    end
end

-- ============================================================================
-- Example 7: Sequential transitions (chain multiple transitions)
-- ============================================================================
function Example7_SequentialTransitions()
    -- First transition
    local pov1 = ViewInfo()
    pov1.Location = Vector(500, 0, 300)
    pov1.Rotation = Quaternion(0, 0, 0, 1)
    pov1.FOV = 90.0

    PlayTransitionPreset(pov1, "QuickCut") -- 0.1s

    -- Schedule second transition after first completes
    -- Note: In actual use, you'd need to use coroutines or timers
    -- This is just for demonstration
    -- Wait(0.1) -- Wait for first transition

    -- local pov2 = ViewInfo()
    -- pov2.Location = Vector(1000, 1000, 500)
    -- pov2.Rotation = Quaternion(0, 0, 0, 1)
    -- pov2.FOV = 75.0
    -- PlayTransitionPreset(pov2, "SlowZoom") -- 2.0s
end

-- ============================================================================
-- Example 8: Using GetCameraManager to access camera directly
-- ============================================================================
function Example8_DirectCameraAccess()
    local cameraManager = GetCameraManager()

    if cameraManager then
        print("Camera manager found!")

        -- Create transition
        local targetPOV = ViewInfo()
        targetPOV.Location = Vector(0, 0, 2000)
        targetPOV.Rotation = Quaternion(0, 0, 0, 1)
        targetPOV.FOV = 60.0

        local curve = BezierEaseOut()
        cameraManager:StartTransition(targetPOV, 1.5, curve)
    else
        print("Camera manager not available (are you in PIE/Game mode?)")
    end
end

-- ============================================================================
-- Available Transition Presets (defined in Engine/Data/CameraTransitionPresets.json)
-- ============================================================================
--
-- "QuickCut"   : Duration=0.1s,  Linear       (instant cut)
-- "SlowZoom"   : Duration=2.0s,  EaseInOut    (slow cinematic zoom)
-- "SmoothPan"  : Duration=1.5s,  EaseOut      (smooth camera pan)
-- "Cinematic"  : Duration=3.0s,  EaseInOut    (cinematic camera move)
-- "Bounce"     : Duration=1.0s,  Bounce       (bouncy transition)
--
-- You can add more presets by editing CameraTransitionPresets.json or using
-- the TransitionPresetManager in C++.
-- ============================================================================

-- ============================================================================
-- Notes:
-- ============================================================================
-- 1. Transitions only work in PIE/Game mode (not in Editor mode)
-- 2. Target POV (FMinimalViewInfo) includes:
--    - Location (FVector)
--    - Rotation (FQuaternion)
--    - FOV (float)
--    - AspectRatio (float)
--    - NearClipPlane (float)
--    - FarClipPlane (float)
-- 3. Multiple transitions can be running, but typically you'd stop previous
--    transitions before starting new ones
-- 4. Bezier curves control the timing (how fast/slow the transition progresses)
-- 5. Use StopAllCameraTransitions() if you need to cancel mid-transition
-- ============================================================================
