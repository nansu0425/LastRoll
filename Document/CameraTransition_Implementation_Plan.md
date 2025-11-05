# Camera Transition System Implementation Plan

**Date**: 2025-11-06
**Author**: Claude
**Status**: Planning Phase
**Target Engine Version**: FutureEngine v0.1

---

## Table of Contents

1. [Overview](#overview)
2. [Current Architecture Analysis](#current-architecture-analysis)
3. [System Design](#system-design)
4. [Implementation Phases](#implementation-phases)
5. [Class Specifications](#class-specifications)
6. [File Structure](#file-structure)
7. [Integration Points](#integration-points)
8. [Code Snippets](#code-snippets)
9. [UI Implementation](#ui-implementation)
10. [Testing Plan](#testing-plan)
11. [Implementation Checklist](#implementation-checklist)

---

## Overview

### Goals

Implement a Camera Transition system that provides smooth, customizable camera movements between different POV states using Bezier curve-based timing control.

**Core Features**:
- **Smooth Camera Transitions**: Interpolate camera position, rotation, and FOV
- **Bezier Curve Timing**: Control transition timing with customizable curves
- **Preset System**: Reusable transition configurations (QuickCut, SlowZoom, SmoothPan, etc.)
- **ImGui Editor Integration**: Visual Bezier curve editor (reuse existing UI)
- **CameraModifier-based**: Non-invasive implementation using existing modifier pipeline

### Design Philosophy

Follow the **Camera Shake System Pattern**:
- ✅ Use `UCameraModifier` base class (same as `UCameraModifier_CameraShake`)
- ✅ Bezier curve for timing control (same as shake decay curve)
- ✅ Preset system with JSON serialization
- ✅ `FImGuiBezierEditor` UI reuse
- ✅ Detail panel with same structure as Camera Shake
- ✅ Lua binding for script-driven transitions

### Key Requirements

1. **Non-Destructive**: Do not modify `APlayerCameraManager` blending logic
2. **Reusable Components**: Leverage existing `FCubicBezierCurve`, `FImGuiBezierEditor`
3. **Consistent Patterns**: Match Camera Shake implementation structure
4. **Extensible**: Support multiple simultaneous transitions (via Priority)

---

## Current Architecture Analysis

### 1. CameraModifier System

**Base Class**: `UCameraModifier`

```cpp
// Engine/Source/Component/Camera/Public/CameraModifier.h
class UCameraModifier : public UObject
{
protected:
    APlayerCameraManager* CameraOwner;
    float AlphaInTime;          // Blend in time
    float AlphaOutTime;         // Blend out time
    float Alpha;                // Current blend weight [0.0, 1.0]
    ECameraModifierBlendMode BlendMode;
    bool bDisabled;
    uint8 Priority;             // Higher = processed later (stronger influence)

public:
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV);
    virtual void UpdateAlpha(float DeltaTime);
};
```

**Key Points**:
- Modifiers are processed in **priority order** by `APlayerCameraManager::ApplyCameraModifiers()`
- Each modifier modifies the `FMinimalViewInfo` POV struct
- Alpha blending automatically handled by base class

### 2. Camera Shake Implementation (Reference Pattern)

**Class**: `UCameraModifier_CameraShake`

```cpp
// Engine/Source/Component/Camera/Public/CameraModifier_CameraShake.h
class UCameraModifier_CameraShake : public UCameraModifier
{
private:
    float ShakeDuration;
    float ShakeTimeRemaining;
    float LocationAmplitude;
    float RotationAmplitude;
    ECameraShakePattern Pattern;
    bool bIsShaking;

    // Bezier curve-based decay
    FCubicBezierCurve DecayCurve;
    bool bUseDecayCurve;

public:
    void StartShake(float InDuration, float InLocationAmplitude, ...);
    void StartShakeWithCurve(float InDuration, ..., const FCubicBezierCurve& InDecayCurve);

    const FCubicBezierCurve& GetDecayCurve() const;
    void SetDecayCurve(const FCubicBezierCurve& InCurve);

    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
};
```

**Pattern to Replicate**:
- Start/Stop API
- Bezier curve for time-based control
- Getter/Setter for curve
- `ModifyCamera()` override for POV manipulation

### 3. Bezier Curve System

**Struct**: `FCubicBezierCurve`

```cpp
// Engine/Source/Global/Public/BezierCurve.h
struct FCubicBezierCurve
{
    FVector2 P[4];  // 4 control points (P0, P1, P2, P3)

    FVector2 Evaluate(float t) const;           // Evaluate curve at t
    float SampleY(float x, int32 iterations = 8) const;  // X → Y mapping
    void Serialize(bool bInIsLoading, JSON& InOutHandle);

    // Preset factory methods
    static FCubicBezierCurve CreateLinear();
    static FCubicBezierCurve CreateEaseIn();
    static FCubicBezierCurve CreateEaseOut();
    static FCubicBezierCurve CreateEaseInOut();
    static FCubicBezierCurve CreateBounce();
};
```

**Usage in Transition**:
- X-axis: Normalized time [0, 1] (ElapsedTime / Duration)
- Y-axis: Blend alpha [0, 1] (interpolation weight)
- `SampleY(normalizedTime)` returns blend alpha for POV interpolation

### 4. PlayerCameraManager Blending (Current Limitation)

**Existing Blending**:

```cpp
// Engine/Source/Actor/Private/PlayerCameraManager.cpp:317-349
void APlayerCameraManager::UpdateBlending(float DeltaTime)
{
    float BlendAlpha = 1.0f - (BlendTimeRemaining / BlendTime);  // Linear!

    CachedPOV.Location = Lerp(ViewTarget.POV.Location, PendingViewTarget.POV.Location, BlendAlpha);
    // Quaternion blending is naive (component-wise lerp, not slerp)
    CachedPOV.Rotation.X = Lerp(ViewTarget.POV.Rotation.X, PendingViewTarget.POV.Rotation.X, BlendAlpha);
    // ...
}
```

**Limitations**:
- ❌ Only linear blending (no Bezier curve support)
- ❌ Naive rotation blending (should use Slerp)
- ❌ Hardcoded in PlayerCameraManager

**Why Not Modify This?**:
- Risk breaking existing camera system
- PlayerCameraManager is core infrastructure
- CameraModifier approach is non-invasive and extensible

### 5. ImGui Bezier Editor

**Component**: `FImGuiBezierEditor`

```cpp
// Engine/Source/ImGui/ImGuiBezierEditor.h (inferred from usage)
class FImGuiBezierEditor
{
public:
    // Returns true if curve was modified
    bool Draw(const char* label, FCubicBezierCurve& curve);
};
```

**Used In**:
- `FCameraShakeDetailPanel::DrawBezierEditor()` (line 50)

**UI Features**:
- Interactive control point dragging
- Visual curve preview
- Preset buttons (Linear, EaseIn, EaseOut, etc.)

---

## System Design

### Architecture Overview

```
[Transition Request]
    ↓
UCameraModifier_Transition::StartTransition(FromPOV, ToPOV, Duration, TimingCurve)
    ↓
APlayerCameraManager::ModifierList (Priority-sorted)
    ↓
APlayerCameraManager::ApplyCameraModifiers(DeltaTime)
    ↓
UCameraModifier_Transition::ModifyCamera(DeltaTime, InOutPOV)
    ↓ (Compute blend alpha from Bezier curve)
    ↓ (Interpolate Location, Rotation, FOV)
FMinimalViewInfo (Interpolated POV)
    ↓
Renderer (Final View/Projection matrices)
```

### Key Design Decisions

#### 1. CameraModifier vs. PlayerCameraManager Modification

| Approach | Pros | Cons |
|----------|------|------|
| **Modify PlayerCameraManager** | Direct control, no extra classes | Risky, invasive, breaks existing code |
| **CameraModifier-based** ✅ | Non-invasive, extensible, reuses existing infrastructure | Slightly more complex API |

**Decision**: Use `UCameraModifier_Transition` for safety and consistency.

#### 2. POV Capture Strategy

**Problem**: How to capture "From" POV when starting transition?

**Options**:
1. **Explicit POV Parameter**: User provides both `FromPOV` and `ToPOV`
2. **Auto-Capture Current**: Capture `CameraManager->GetCameraCachePOV()` as `FromPOV`

**Decision**: **Explicit POV** for maximum flexibility. User code:
```cpp
FMinimalViewInfo CurrentPOV = CameraManager->GetCameraCachePOV();
FMinimalViewInfo TargetPOV = /* ... */;
Transition->StartTransition(CurrentPOV, TargetPOV, Duration, Curve);
```

#### 3. Rotation Interpolation

**Current PlayerCameraManager**: Component-wise Lerp (incorrect)

**Correct Method**: Quaternion Slerp

```cpp
// In UCameraModifier_Transition::ModifyCamera()
InOutPOV.Rotation = FQuaternion::Slerp(StartPOV.Rotation, TargetPOV.Rotation, BlendAlpha);
```

### Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────┐
│ APlayerCameraManager                                        │
│                                                             │
│  ModifierList: [UCameraModifier_Transition, ...]           │
│                                                             │
│  UpdateCamera(DeltaTime)                                    │
│    ├─ UpdateViewTarget(DeltaTime)                          │
│    ├─ UpdateBlending(DeltaTime)                            │
│    ├─ ApplyCameraModifiers(DeltaTime)  <────┐              │
│    │   └─ For each modifier (priority order)│              │
│    │       ├─ UpdateAlpha(DeltaTime)         │              │
│    │       └─ ModifyCamera(DeltaTime, POV) ──┼─────────┐   │
│    ├─ UpdateFading(DeltaTime)                │         │   │
│    └─ UpdateCameraConstants()                │         │   │
└───────────────────────────────────────────────┼─────────┼───┘
                                                │         │
                ┌───────────────────────────────┘         │
                ↓                                         │
┌─────────────────────────────────────────────────────────┼───┐
│ UCameraModifier_Transition                              │   │
│                                                         ↓   │
│  Private:                                    ModifyCamera() │
│    - TransitionDuration                          │          │
│    - TransitionTimeRemaining                     │          │
│    - bIsTransitioning                            │          │
│    - StartPOV                                    │          │
│    - TargetPOV                                   │          │
│    - TimingCurve (FCubicBezierCurve)             │          │
│    - bUseTimingCurve                             │          │
│                                                  ↓          │
│  Public:                         ┌──────────────────────┐  │
│    StartTransition(...)          │ 1. Update time       │  │
│    StopTransition()              │ 2. Check completion  │  │
│    IsTransitioning()             │ 3. Calc blend alpha  │  │
│    GetTimingCurve()              │    (Bezier.SampleY)  │  │
│    SetTimingCurve(...)           │ 4. Interpolate POV   │  │
│                                  │    - Location (Lerp) │  │
│                                  │    - Rotation (Slerp)│  │
│                                  │    - FOV (Lerp)      │  │
│                                  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                │
                │ Uses
                ↓
┌─────────────────────────────────────────────────────────────┐
│ FCubicBezierCurve                                           │
│                                                             │
│  P[0], P[1], P[2], P[3] (Control Points)                   │
│                                                             │
│  float SampleY(float x)  ──→  Newton-Raphson solver        │
│      Input:  x = ElapsedTime / Duration  [0, 1]            │
│      Output: y = Blend Alpha             [0, 1]            │
│                                                             │
│  Presets: CreateEaseIn(), CreateEaseOut(), ...             │
└─────────────────────────────────────────────────────────────┘
```

---

## Implementation Phases

### Phase 1: Core Transition Modifier

**Tasks**:
1. Create `UCameraModifier_Transition` class
2. Implement `StartTransition()`, `StopTransition()` API
3. Implement `ModifyCamera()` with Bezier curve blending
4. Add proper Quaternion Slerp for rotation
5. Basic unit testing (manual C++ test)

**Files to Create**:
- `Engine/Source/Component/Camera/Public/CameraModifier_Transition.h`
- `Engine/Source/Component/Camera/Private/CameraModifier_Transition.cpp`

**Deliverable**: Working transition modifier, callable from C++

---

### Phase 2: Preset System

**Tasks**:
1. Create `FTransitionPresetData` struct
2. Implement `UTransitionPresetManager` singleton
3. Add JSON serialization
4. Create default presets (QuickCut, SlowZoom, SmoothPan, etc.)
5. Implement Save/Load from `Data/CameraTransitionPresets.json`

**Files to Create**:
- `Engine/Source/Global/Public/TransitionTypes.h` (FTransitionPresetData)
- `Engine/Source/Manager/Camera/Public/TransitionPresetManager.h`
- `Engine/Source/Manager/Camera/Private/TransitionPresetManager.cpp`
- `Engine/Data/CameraTransitionPresets.json` (default presets)

**Deliverable**: Reusable transition presets with persistence

---

### Phase 3: ImGui Editor UI

**Tasks**:
1. Create `FTransitionDetailPanel` class
2. Implement Bezier editor section (reuse `FImGuiBezierEditor`)
3. Add Preset buttons (Linear, EaseIn, EaseOut, etc.)
4. Implement curve preview graph
5. Add transition parameter controls (Duration, etc.)
6. Add Test controls (Start/Stop buttons for PIE mode)
7. Integrate into Editor Window or Camera Manager inspector

**Files to Create**:
- `Engine/Source/Editor/Public/TransitionDetailPanel.h`
- `Engine/Source/Editor/Private/TransitionDetailPanel.cpp`

**Deliverable**: Visual editor for transition curves

---

### Phase 4: Lua Binding

**Tasks**:
1. Register `UCameraModifier_Transition` type in `ScriptManager.cpp`
2. Add Lua API for starting/stopping transitions
3. Add helper functions for common use cases
4. Document Lua API

**Files to Modify**:
- `Engine/Source/Manager/Script/Private/ScriptManager.cpp`

**Deliverable**: Lua-accessible transition API

---

### Phase 5: Helper APIs & Polish

**Tasks**:
1. Add `APlayerCameraManager::StartTransition()` helper
2. Add `APlayerCameraManager::PlayTransitionPreset(FName)` API
3. Implement transition queuing (optional)
4. Add debug visualization (draw transition path in editor)
5. Performance profiling

**Deliverable**: Production-ready API

---

## Class Specifications

### 1. UCameraModifier_Transition

**Header**: `Engine/Source/Component/Camera/Public/CameraModifier_Transition.h`

```cpp
#pragma once
#include "Component/Camera/Public/CameraModifier.h"
#include "Global/Public/BezierCurve.h"

/**
 * @brief Camera Transition Modifier - Smooth camera POV interpolation
 *
 * Provides smooth transitions between camera states using Bezier curve-based timing.
 * Interpolates Location, Rotation (Slerp), and FOV over a specified duration.
 *
 * Usage Example:
 * ```cpp
 * UCameraModifier_Transition* Transition = Cast<UCameraModifier_Transition>(
 *     CameraManager->AddCameraModifier(UCameraModifier_Transition::StaticClass())
 * );
 *
 * FMinimalViewInfo CurrentPOV = CameraManager->GetCameraCachePOV();
 * FMinimalViewInfo TargetPOV;
 * TargetPOV.Location = FVector(100, 200, 300);
 * TargetPOV.Rotation = FQuaternion::FromEuler(FVector(0, 45, 0));
 * TargetPOV.FOV = 60.0f;
 *
 * FCubicBezierCurve Curve = FCubicBezierCurve::CreateEaseInOut();
 * Transition->StartTransition(CurrentPOV, TargetPOV, 2.0f, Curve);
 * ```
 */
UCLASS()
class UCameraModifier_Transition : public UCameraModifier
{
    GENERATED_BODY()
    DECLARE_CLASS(UCameraModifier_Transition, UCameraModifier)

private:
    // ===== Transition State =====
    float TransitionDuration;         // Total transition duration (seconds)
    float TransitionTimeRemaining;    // Time remaining until completion
    float TransitionTime;             // Elapsed time since start
    bool bIsTransitioning;            // Is transition currently active?

    // ===== POV States =====
    FMinimalViewInfo StartPOV;        // Starting camera state
    FMinimalViewInfo TargetPOV;       // Target camera state

    // ===== Timing Curve =====
    FCubicBezierCurve TimingCurve;    // Bezier curve for timing control
                                      // X = Normalized time [0,1]
                                      // Y = Blend alpha [0,1]
    bool bUseTimingCurve;             // If false, use linear interpolation

public:
    UCameraModifier_Transition();
    virtual ~UCameraModifier_Transition() override;

    /**
     * @brief Start a camera transition
     *
     * @param InFromPOV Starting camera state (usually current camera)
     * @param InToPOV Target camera state
     * @param InDuration Transition duration in seconds (must be > 0)
     * @param InTimingCurve Bezier curve controlling interpolation timing
     */
    void StartTransition(
        const FMinimalViewInfo& InFromPOV,
        const FMinimalViewInfo& InToPOV,
        float InDuration,
        const FCubicBezierCurve& InTimingCurve
    );

    /**
     * @brief Start a transition with linear timing (no curve)
     */
    void StartTransitionLinear(
        const FMinimalViewInfo& InFromPOV,
        const FMinimalViewInfo& InToPOV,
        float InDuration
    );

    /**
     * @brief Stop the current transition immediately
     *
     * Camera will snap to target state.
     */
    void StopTransition();

    /**
     * @brief Check if a transition is currently active
     */
    bool IsTransitioning() const { return bIsTransitioning; }

    /**
     * @brief Get remaining transition time
     */
    float GetTimeRemaining() const { return TransitionTimeRemaining; }

    /**
     * @brief Get transition progress [0, 1]
     */
    float GetProgress() const
    {
        return bIsTransitioning ? (TransitionTime / TransitionDuration) : 0.0f;
    }

    // ===== Bezier Curve Access =====

    /**
     * @brief Get the current timing curve
     */
    const FCubicBezierCurve& GetTimingCurve() const { return TimingCurve; }

    /**
     * @brief Set the timing curve
     *
     * @param InCurve New timing curve
     * @note Only affects new transitions, does not modify active transition
     */
    void SetTimingCurve(const FCubicBezierCurve& InCurve);

    /**
     * @brief Check if using Bezier curve timing
     */
    bool IsUsingTimingCurve() const { return bUseTimingCurve; }

    /**
     * @brief Enable/disable Bezier curve timing
     *
     * @param bInUse true = use TimingCurve, false = linear interpolation
     */
    void SetUseTimingCurve(bool bInUse) { bUseTimingCurve = bInUse; }

    // ===== UCameraModifier Overrides =====

    virtual void Initialize(APlayerCameraManager* InOwner) override;
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
    /**
     * @brief Compute blend alpha from normalized time
     *
     * @param NormalizedTime Time ratio [0, 1]
     * @return Blend alpha [0, 1]
     */
    float ComputeBlendAlpha(float NormalizedTime) const;
};
```

### 2. FTransitionPresetData

**Header**: `Engine/Source/Global/Public/TransitionTypes.h`

```cpp
#pragma once
#include "Global/Types.h"
#include "Global/Public/BezierCurve.h"

namespace json { class JSON; }
using JSON = json::JSON;

/**
 * @brief Camera Transition Preset Data
 *
 * Stores reusable transition configurations.
 * Presets are saved/loaded from JSON files and managed by UTransitionPresetManager.
 *
 * Example Presets:
 * - "QuickCut": Duration=0.1s, Linear
 * - "SlowZoom": Duration=2.0s, EaseInOut
 * - "SmoothPan": Duration=1.5s, EaseOut
 * - "Cinematic": Duration=3.0s, Custom Bezier
 */
struct FTransitionPresetData
{
    /**
     * @brief Preset name (unique identifier)
     *
     * Examples: "QuickCut", "SlowZoom", "SmoothPan"
     */
    FName PresetName;

    /**
     * @brief Transition duration (seconds)
     *
     * Range: 0.1 ~ 10.0
     * Default: 1.0
     */
    float Duration = 1.0f;

    /**
     * @brief Use Bezier curve timing
     *
     * true: Use TimingCurve
     * false: Linear interpolation
     */
    bool bUseTimingCurve = true;

    /**
     * @brief Bezier curve for timing control
     *
     * Only used if bUseTimingCurve == true.
     * X = Normalized time [0,1]
     * Y = Blend alpha [0,1]
     */
    FCubicBezierCurve TimingCurve;

    /**
     * @brief Default constructor
     */
    FTransitionPresetData();

    /**
     * @brief JSON serialization
     *
     * @param bIsLoading true = load from JSON, false = save to JSON
     * @param InOutHandle JSON handle
     */
    void Serialize(bool bIsLoading, JSON& InOutHandle);
};
```

### 3. UTransitionPresetManager

**Header**: `Engine/Source/Manager/Camera/Public/TransitionPresetManager.h`

```cpp
#pragma once
#include "Core/Public/Object.h"
#include "Global/Public/TransitionTypes.h"

/**
 * @brief Transition Preset Manager Singleton
 *
 * Manages a library of reusable transition presets.
 * Presets are loaded from/saved to JSON files.
 *
 * Default File: Engine/Data/CameraTransitionPresets.json
 */
UCLASS()
class UTransitionPresetManager : public UObject
{
    GENERATED_BODY()
    DECLARE_SINGLETON_CLASS(UTransitionPresetManager, UObject)

private:
    TMap<FName, FTransitionPresetData> PresetMap;

public:
    UTransitionPresetManager();
    virtual ~UTransitionPresetManager() override;

    /**
     * @brief Initialize manager (load default presets)
     */
    void Initialize();

    /**
     * @brief Load presets from JSON file
     *
     * @param FilePath Absolute or relative path to preset file
     * @return true if successful
     */
    bool LoadPresetsFromFile(const FString& FilePath);

    /**
     * @brief Save presets to JSON file
     *
     * @param FilePath Absolute or relative path to preset file
     * @return true if successful
     */
    bool SavePresetsToFile(const FString& FilePath);

    /**
     * @brief Find preset by name
     *
     * @param PresetName Preset identifier
     * @return Pointer to preset data, or nullptr if not found
     */
    FTransitionPresetData* FindPreset(FName PresetName);

    /**
     * @brief Add or update preset
     *
     * @param Preset Preset data to add
     * @note If preset with same name exists, it will be replaced
     */
    void AddPreset(const FTransitionPresetData& Preset);

    /**
     * @brief Remove preset by name
     *
     * @param PresetName Preset to remove
     * @return true if preset was found and removed
     */
    bool RemovePreset(FName PresetName);

    /**
     * @brief Get all preset names
     *
     * @return Array of preset names
     */
    TArray<FName> GetAllPresetNames() const;

    /**
     * @brief Clear all presets
     */
    void ClearAllPresets();

    /**
     * @brief Create default presets (QuickCut, SlowZoom, etc.)
     */
    void CreateDefaultPresets();
};
```

### 4. FTransitionDetailPanel

**Header**: `Engine/Source/Editor/Public/TransitionDetailPanel.h`

```cpp
#pragma once
#include "Component/Camera/Public/CameraModifier_Transition.h"
#include "ImGui/ImGuiBezierEditor.h"

class UWorld;

/**
 * @brief Transition Detail Panel for ImGui
 *
 * Provides visual editor for UCameraModifier_Transition:
 * - Bezier curve editor (timing curve)
 * - Preset buttons (Linear, EaseIn, EaseOut, EaseInOut, Bounce)
 * - Transition parameters (Duration)
 * - Curve preview graph
 * - Test controls (Start/Stop transition in PIE mode)
 *
 * Usage Example:
 * ```cpp
 * FTransitionDetailPanel Panel;
 * if (UCameraModifier_Transition* Transition = GetSelectedTransition())
 * {
 *     Panel.Draw("TransitionDetail", Transition);
 * }
 * ```
 */
class FTransitionDetailPanel
{
public:
    FTransitionDetailPanel();
    ~FTransitionDetailPanel();

    /**
     * @brief Render detail panel UI
     *
     * @param Label Panel unique ID (for ImGui)
     * @param Transition Transition modifier to edit
     * @param World Current world (for PIE mode check)
     * @return true if parameters were modified
     */
    bool Draw(const char* Label, UCameraModifier_Transition* Transition, UWorld* World);

private:
    /**
     * @brief Bezier curve editor section
     */
    bool DrawBezierEditor(UCameraModifier_Transition* Transition);

    /**
     * @brief Preset buttons section
     */
    bool DrawPresetButtons(UCameraModifier_Transition* Transition);

    /**
     * @brief Curve preview graph
     */
    void DrawCurvePreview(UCameraModifier_Transition* Transition);

    /**
     * @brief Transition parameters editor
     */
    bool DrawTransitionParameters(UCameraModifier_Transition* Transition);

    /**
     * @brief Test controls (Start/Stop buttons)
     */
    void DrawTestControls(UCameraModifier_Transition* Transition, UWorld* World);

private:
    // Bezier editor instance (reused from Camera Shake)
    FImGuiBezierEditor BezierEditor;

    // Editor state
    bool bShowPreview;          // Show preview graph?
    float PreviewSampleX;       // Preview sampling X [0, 1]

    // Temporary parameters (UI editing)
    float TempDuration;
    bool bTempUseTimingCurve;

    // UI style
    float PresetButtonWidth;
    float PreviewGraphHeight;
};
```

---

## File Structure

### New Files to Create

```
Engine/Source/
├── Component/Camera/
│   ├── Public/
│   │   └── CameraModifier_Transition.h          [Phase 1]
│   └── Private/
│       └── CameraModifier_Transition.cpp        [Phase 1]
│
├── Global/Public/
│   └── TransitionTypes.h                        [Phase 2]
│
├── Manager/Camera/
│   ├── Public/
│   │   └── TransitionPresetManager.h            [Phase 2]
│   └── Private/
│       └── TransitionPresetManager.cpp          [Phase 2]
│
└── Editor/
    ├── Public/
    │   └── TransitionDetailPanel.h              [Phase 3]
    └── Private/
        └── TransitionDetailPanel.cpp            [Phase 3]

Engine/Data/
└── CameraTransitionPresets.json                 [Phase 2]

Document/
└── CameraTransition_Implementation_Plan.md     [Current File]
```

### Files to Modify

```
Engine/Source/
├── Manager/Script/Private/
│   └── ScriptManager.cpp                        [Phase 4] (Lua binding)
│
└── Actor/Public/
    └── PlayerCameraManager.h                    [Phase 5] (Helper APIs)
```

---

## Integration Points

### 1. PlayerCameraManager Integration

**File**: `Engine/Source/Actor/Public/PlayerCameraManager.h`

Add helper methods (Phase 5):

```cpp
class APlayerCameraManager : public AActor
{
public:
    /**
     * @brief Start a camera transition with Bezier timing
     *
     * Helper method that automatically captures current camera state.
     *
     * @param TargetPOV Target camera state
     * @param Duration Transition duration (seconds)
     * @param TimingCurve Bezier curve for timing
     * @return Created transition modifier instance
     */
    UCameraModifier_Transition* StartTransition(
        const FMinimalViewInfo& TargetPOV,
        float Duration,
        const FCubicBezierCurve& TimingCurve
    );

    /**
     * @brief Start a transition using a preset
     *
     * @param TargetPOV Target camera state
     * @param PresetName Preset identifier (e.g., "SlowZoom")
     * @return Created transition modifier, or nullptr if preset not found
     */
    UCameraModifier_Transition* PlayTransitionPreset(
        const FMinimalViewInfo& TargetPOV,
        FName PresetName
    );

    /**
     * @brief Stop all active transitions
     */
    void StopAllTransitions();
};
```

Implementation:

```cpp
// PlayerCameraManager.cpp
UCameraModifier_Transition* APlayerCameraManager::StartTransition(
    const FMinimalViewInfo& TargetPOV,
    float Duration,
    const FCubicBezierCurve& TimingCurve)
{
    // Find or create transition modifier
    UCameraModifier_Transition* Transition = Cast<UCameraModifier_Transition>(
        FindCameraModifierByClass(UCameraModifier_Transition::StaticClass())
    );

    if (!Transition)
    {
        Transition = Cast<UCameraModifier_Transition>(
            AddCameraModifier(UCameraModifier_Transition::StaticClass())
        );
    }

    if (Transition)
    {
        FMinimalViewInfo CurrentPOV = GetCameraCachePOV();
        Transition->StartTransition(CurrentPOV, TargetPOV, Duration, TimingCurve);
    }

    return Transition;
}

UCameraModifier_Transition* APlayerCameraManager::PlayTransitionPreset(
    const FMinimalViewInfo& TargetPOV,
    FName PresetName)
{
    UTransitionPresetManager& PresetManager = UTransitionPresetManager::GetInstance();
    FTransitionPresetData* Preset = PresetManager.FindPreset(PresetName);

    if (!Preset)
    {
        UE_LOG_ERROR("Transition preset '%s' not found", PresetName.ToString().c_str());
        return nullptr;
    }

    return StartTransition(TargetPOV, Preset->Duration, Preset->TimingCurve);
}
```

### 2. Lua Binding

**File**: `Engine/Source/Manager/Script/Private/ScriptManager.cpp`

Add to `RegisterCoreTypes()` (Phase 4):

```cpp
// Register FMinimalViewInfo struct
lua.new_usertype<FMinimalViewInfo>("ViewInfo",
    sol::constructors<FMinimalViewInfo()>(),
    "Location", &FMinimalViewInfo::Location,
    "Rotation", &FMinimalViewInfo::Rotation,
    "FOV", &FMinimalViewInfo::FOV,
    "AspectRatio", &FMinimalViewInfo::AspectRatio,
    "NearClipPlane", &FMinimalViewInfo::NearClipPlane,
    "FarClipPlane", &FMinimalViewInfo::FarClipPlane
);

// Register UCameraModifier_Transition
lua.new_usertype<UCameraModifier_Transition>("CameraTransition",
    "StartTransition", &UCameraModifier_Transition::StartTransition,
    "StartTransitionLinear", &UCameraModifier_Transition::StartTransitionLinear,
    "StopTransition", &UCameraModifier_Transition::StopTransition,
    "IsTransitioning", &UCameraModifier_Transition::IsTransitioning,
    "GetProgress", &UCameraModifier_Transition::GetProgress,
    "SetTimingCurve", &UCameraModifier_Transition::SetTimingCurve,
    "GetTimingCurve", &UCameraModifier_Transition::GetTimingCurve
);

// Helper: Get PlayerCameraManager from Player
LuaState["GetCameraManager"] = []() -> APlayerCameraManager* {
    if (GWorld && (GWorld->GetWorldType() == EWorldType::PIE ||
                   GWorld->GetWorldType() == EWorldType::Game))
    {
        return GWorld->GetCameraManager();
    }
    return nullptr;
};
```

**Lua Usage Example**:

```lua
-- Player.lua
function StartCinematicTransition()
    local CameraManager = GetCameraManager()
    if not CameraManager then return end

    -- Create target POV
    local TargetPOV = ViewInfo()
    TargetPOV.Location = Vector(100, 200, 300)
    TargetPOV.Rotation = Quaternion.FromEuler(Vector(0, 45, 0))
    TargetPOV.FOV = 60.0

    -- Create EaseInOut curve
    local Curve = BezierCurve.CreateEaseInOut()

    -- Start 2-second transition
    CameraManager:PlayTransitionPreset(TargetPOV, "SlowZoom")
end
```

---

## Code Snippets

### 1. UCameraModifier_Transition::ModifyCamera() Implementation

```cpp
// CameraModifier_Transition.cpp
bool UCameraModifier_Transition::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    if (!bIsTransitioning)
        return false;

    // Update transition time
    TransitionTimeRemaining -= DeltaTime;
    TransitionTime += DeltaTime;

    if (TransitionTimeRemaining <= 0.0f)
    {
        // Transition completed - snap to target
        InOutPOV.Location = TargetPOV.Location;
        InOutPOV.Rotation = TargetPOV.Rotation;
        InOutPOV.FOV = TargetPOV.FOV;
        InOutPOV.AspectRatio = TargetPOV.AspectRatio;
        InOutPOV.NearClipPlane = TargetPOV.NearClipPlane;
        InOutPOV.FarClipPlane = TargetPOV.FarClipPlane;

        bIsTransitioning = false;
        TransitionTimeRemaining = 0.0f;

        UE_LOG_INFO("Camera transition completed");
        return true;
    }

    // Compute normalized time [0, 1]
    float NormalizedTime = TransitionTime / TransitionDuration;

    // Compute blend alpha from timing curve
    float BlendAlpha = ComputeBlendAlpha(NormalizedTime);

    // Interpolate POV
    InOutPOV.Location = Lerp(StartPOV.Location, TargetPOV.Location, BlendAlpha);

    // IMPORTANT: Use Quaternion Slerp for smooth rotation
    InOutPOV.Rotation = FQuaternion::Slerp(StartPOV.Rotation, TargetPOV.Rotation, BlendAlpha);

    InOutPOV.FOV = Lerp(StartPOV.FOV, TargetPOV.FOV, BlendAlpha);
    InOutPOV.AspectRatio = Lerp(StartPOV.AspectRatio, TargetPOV.AspectRatio, BlendAlpha);
    InOutPOV.NearClipPlane = Lerp(StartPOV.NearClipPlane, TargetPOV.NearClipPlane, BlendAlpha);
    InOutPOV.FarClipPlane = Lerp(StartPOV.FarClipPlane, TargetPOV.FarClipPlane, BlendAlpha);

    return true;
}

float UCameraModifier_Transition::ComputeBlendAlpha(float NormalizedTime) const
{
    if (bUseTimingCurve)
    {
        // Sample Y from Bezier curve based on X (normalized time)
        return TimingCurve.SampleY(NormalizedTime);
    }
    else
    {
        // Linear interpolation
        return NormalizedTime;
    }
}
```

### 2. Default Presets Creation

```cpp
// TransitionPresetManager.cpp
void UTransitionPresetManager::CreateDefaultPresets()
{
    // QuickCut: Instant transition (0.1s, linear)
    {
        FTransitionPresetData Preset;
        Preset.PresetName = FName("QuickCut");
        Preset.Duration = 0.1f;
        Preset.bUseTimingCurve = false;
        AddPreset(Preset);
    }

    // SlowZoom: Slow zoom effect (2s, EaseInOut)
    {
        FTransitionPresetData Preset;
        Preset.PresetName = FName("SlowZoom");
        Preset.Duration = 2.0f;
        Preset.bUseTimingCurve = true;
        Preset.TimingCurve = FCubicBezierCurve::CreateEaseInOut();
        AddPreset(Preset);
    }

    // SmoothPan: Smooth camera pan (1.5s, EaseOut)
    {
        FTransitionPresetData Preset;
        Preset.PresetName = FName("SmoothPan");
        Preset.Duration = 1.5f;
        Preset.bUseTimingCurve = true;
        Preset.TimingCurve = FCubicBezierCurve::CreateEaseOut();
        AddPreset(Preset);
    }

    // Cinematic: Cinematic camera move (3s, EaseInOut)
    {
        FTransitionPresetData Preset;
        Preset.PresetName = FName("Cinematic");
        Preset.Duration = 3.0f;
        Preset.bUseTimingCurve = true;
        Preset.TimingCurve = FCubicBezierCurve::CreateEaseInOut();
        AddPreset(Preset);
    }

    // Bounce: Bouncy transition (1s, Bounce)
    {
        FTransitionPresetData Preset;
        Preset.PresetName = FName("Bounce");
        Preset.Duration = 1.0f;
        Preset.bUseTimingCurve = true;
        Preset.TimingCurve = FCubicBezierCurve::CreateBounce();
        AddPreset(Preset);
    }

    UE_LOG_SUCCESS("Created %d default transition presets", 5);
}
```

### 3. Bezier Editor Integration (Detail Panel)

```cpp
// TransitionDetailPanel.cpp
bool FTransitionDetailPanel::DrawBezierEditor(UCameraModifier_Transition* Transition)
{
    bool bModified = false;

    ImGui::Text("Timing Curve");
    ImGui::Separator();

    // Use Bezier Curve checkbox
    bool bUseCurve = Transition->IsUsingTimingCurve();
    if (ImGui::Checkbox("Use Bezier Curve", &bUseCurve))
    {
        Transition->SetUseTimingCurve(bUseCurve);
        bModified = true;
    }

    if (bUseCurve)
    {
        // Draw Bezier editor (REUSE from Camera Shake!)
        FCubicBezierCurve Curve = Transition->GetTimingCurve();
        if (BezierEditor.Draw("TransitionTimingCurve", Curve))
        {
            Transition->SetTimingCurve(Curve);
            bModified = true;
        }
    }

    return bModified;
}

bool FTransitionDetailPanel::DrawPresetButtons(UCameraModifier_Transition* Transition)
{
    bool bModified = false;

    ImGui::Text("Curve Presets");

    // Button layout: [Linear] [EaseIn] [EaseOut] [EaseInOut] [Bounce]
    if (ImGui::Button("Linear", ImVec2(PresetButtonWidth, 0)))
    {
        Transition->SetTimingCurve(FCubicBezierCurve::CreateLinear());
        bModified = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("EaseIn", ImVec2(PresetButtonWidth, 0)))
    {
        Transition->SetTimingCurve(FCubicBezierCurve::CreateEaseIn());
        bModified = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("EaseOut", ImVec2(PresetButtonWidth, 0)))
    {
        Transition->SetTimingCurve(FCubicBezierCurve::CreateEaseOut());
        bModified = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("EaseInOut", ImVec2(PresetButtonWidth, 0)))
    {
        Transition->SetTimingCurve(FCubicBezierCurve::CreateEaseInOut());
        bModified = true;
    }
    ImGui::SameLine();

    if (ImGui::Button("Bounce", ImVec2(PresetButtonWidth, 0)))
    {
        Transition->SetTimingCurve(FCubicBezierCurve::CreateBounce());
        bModified = true;
    }

    return bModified;
}
```

---

## UI Implementation

### ImGui Detail Panel Layout

```
┌─────────────────────────────────────────────────────────────┐
│ Camera Transition Settings                                  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ Transition Parameters                                       │
│ ─────────────────────────────────────────────────────────── │
│   Duration:  [  2.0  ] seconds                              │
│   [ ] Use Bezier Curve                                      │
│                                                             │
│ Timing Curve                                                │
│ ─────────────────────────────────────────────────────────── │
│   ┌───────────────────────────────────────────────────┐    │
│   │                                                   │    │
│   │         [Bezier Curve Editor Widget]             │    │
│   │                                                   │    │
│   │     • Draggable control points                    │    │
│   │     • Visual curve preview                        │    │
│   │     • Grid background                             │    │
│   │                                                   │    │
│   └───────────────────────────────────────────────────┘    │
│                                                             │
│   Curve Presets:                                            │
│   [Linear] [EaseIn] [EaseOut] [EaseInOut] [Bounce]        │
│                                                             │
│ Curve Preview                                               │
│ ─────────────────────────────────────────────────────────── │
│   Sample X: [  0.5  ]                                       │
│   Sample Y:    0.75                                         │
│                                                             │
│   ┌───────────────────────────────────────────────────┐    │
│   │ Y ▲                                               │    │
│   │ 1.0│          ╱─────                              │    │
│   │    │        ╱                                     │    │
│   │ 0.5│      ╱                                       │    │
│   │    │    ╱                                         │    │
│   │ 0.0└────────────────────────────▶ X               │    │
│   │    0.0  0.25  0.5  0.75  1.0                      │    │
│   └───────────────────────────────────────────────────┘    │
│                                                             │
│ Test Controls (PIE Mode Only)                              │
│ ─────────────────────────────────────────────────────────── │
│   [Start Transition]  [Stop Transition]                    │
│                                                             │
│   Status: Transitioning (Progress: 45%)                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Testing Plan

### Unit Tests

**File**: `Engine/Tests/CameraTransitionTests.cpp`

```cpp
// Test 1: Basic Transition
void TestBasicTransition()
{
    UCameraModifier_Transition* Transition = NewObject<UCameraModifier_Transition>();

    FMinimalViewInfo StartPOV;
    StartPOV.Location = FVector(0, 0, 0);

    FMinimalViewInfo TargetPOV;
    TargetPOV.Location = FVector(100, 0, 0);

    FCubicBezierCurve Curve = FCubicBezierCurve::CreateLinear();
    Transition->StartTransition(StartPOV, TargetPOV, 1.0f, Curve);

    assert(Transition->IsTransitioning());
}

// Test 2: Bezier Curve Evaluation
void TestBezierTiming()
{
    FCubicBezierCurve EaseIn = FCubicBezierCurve::CreateEaseIn();

    float y0 = EaseIn.SampleY(0.0f);
    float y1 = EaseIn.SampleY(1.0f);

    assert(fabs(y0 - 0.0f) < 0.01f);
    assert(fabs(y1 - 1.0f) < 0.01f);
}

// Test 3: Quaternion Slerp
void TestRotationInterpolation()
{
    FQuaternion Start = FQuaternion::FromEuler(FVector(0, 0, 0));
    FQuaternion End = FQuaternion::FromEuler(FVector(0, 90, 0));

    FQuaternion Mid = FQuaternion::Slerp(Start, End, 0.5f);
    FVector EulerMid = Mid.ToEuler();

    assert(fabs(EulerMid.Y - 45.0f) < 1.0f);  // Should be ~45 degrees
}
```

### Integration Tests

**Scenario 1**: C++ Transition API

```cpp
void TestCppTransition()
{
    APlayerCameraManager* CameraManager = GetCameraManager();

    FMinimalViewInfo TargetPOV;
    TargetPOV.Location = FVector(100, 200, 300);
    TargetPOV.FOV = 60.0f;

    FCubicBezierCurve Curve = FCubicBezierCurve::CreateEaseInOut();
    UCameraModifier_Transition* Transition = CameraManager->StartTransition(TargetPOV, 2.0f, Curve);

    // Wait 1 second (50% progress)
    Sleep(1000);
    float Progress = Transition->GetProgress();
    assert(Progress > 0.4f && Progress < 0.6f);
}
```

**Scenario 2**: Preset System

```cpp
void TestPresetSystem()
{
    UTransitionPresetManager& Manager = UTransitionPresetManager::GetInstance();
    Manager.CreateDefaultPresets();

    FTransitionPresetData* Preset = Manager.FindPreset(FName("SlowZoom"));
    assert(Preset != nullptr);
    assert(Preset->Duration == 2.0f);
    assert(Preset->bUseTimingCurve == true);
}
```

**Scenario 3**: Lua API

```lua
-- Player.lua test
function TestTransitionFromLua()
    local CameraManager = GetCameraManager()
    assert(CameraManager ~= nil)

    local TargetPOV = ViewInfo()
    TargetPOV.Location = Vector(100, 200, 300)

    CameraManager:PlayTransitionPreset(TargetPOV, "SlowZoom")

    print("Transition started from Lua!")
end
```

### Visual Tests (PIE Mode)

1. **Test 1**: Quick Cut
   - Start transition with 0.1s duration
   - Verify camera snaps quickly

2. **Test 2**: Slow Zoom
   - Start transition with 2s duration, EaseInOut
   - Verify smooth acceleration/deceleration

3. **Test 3**: Multiple Transitions
   - Start transition A (Priority 0)
   - Start transition B (Priority 10) while A is active
   - Verify B overrides A

4. **Test 4**: UI Editor
   - Open Transition Detail Panel
   - Modify Bezier curve
   - Start test transition
   - Verify timing matches curve

---

## Implementation Checklist

### Phase 1: Core Transition Modifier ✅

- [ ] Create `CameraModifier_Transition.h`
- [ ] Create `CameraModifier_Transition.cpp`
- [ ] Implement constructor/destructor
- [ ] Implement `Initialize()`
- [ ] Implement `StartTransition()`
- [ ] Implement `StartTransitionLinear()`
- [ ] Implement `StopTransition()`
- [ ] Implement `ModifyCamera()` with Bezier curve blending
- [ ] Implement `ComputeBlendAlpha()`
- [ ] Add Quaternion Slerp for rotation
- [ ] Add Bezier curve getter/setter
- [ ] Test basic transition in C++

### Phase 2: Preset System ✅

- [ ] Create `TransitionTypes.h` (FTransitionPresetData)
- [ ] Implement `FTransitionPresetData::Serialize()`
- [ ] Create `TransitionPresetManager.h`
- [ ] Create `TransitionPresetManager.cpp`
- [ ] Implement singleton pattern
- [ ] Implement `LoadPresetsFromFile()`
- [ ] Implement `SavePresetsToFile()`
- [ ] Implement `FindPreset()`
- [ ] Implement `AddPreset()` / `RemovePreset()`
- [ ] Implement `CreateDefaultPresets()`
- [ ] Create `Data/CameraTransitionPresets.json` with defaults
- [ ] Test preset loading/saving

### Phase 3: ImGui Editor UI ✅

- [ ] Create `TransitionDetailPanel.h`
- [ ] Create `TransitionDetailPanel.cpp`
- [ ] Implement constructor/destructor
- [ ] Implement `Draw()` main function
- [ ] Implement `DrawBezierEditor()` (reuse FImGuiBezierEditor)
- [ ] Implement `DrawPresetButtons()`
- [ ] Implement `DrawCurvePreview()`
- [ ] Implement `DrawTransitionParameters()`
- [ ] Implement `DrawTestControls()`
- [ ] Integrate into Editor Window
- [ ] Test UI in Editor

### Phase 4: Lua Binding ✅

- [ ] Register `FMinimalViewInfo` in Lua
- [ ] Register `UCameraModifier_Transition` in Lua
- [ ] Add `GetCameraManager()` helper function
- [ ] Test Lua transition API
- [ ] Write Lua usage examples

### Phase 5: Helper APIs & Polish ✅

- [ ] Add `APlayerCameraManager::StartTransition()`
- [ ] Add `APlayerCameraManager::PlayTransitionPreset()`
- [ ] Add `APlayerCameraManager::StopAllTransitions()`
- [ ] Implement transition queuing (optional)
- [ ] Add debug visualization (draw transition path)
- [ ] Performance profiling
- [ ] Write documentation
- [ ] Final integration testing

---

## Notes

### Differences from PlayerCameraManager Blending

| Feature | PlayerCameraManager::UpdateBlending() | UCameraModifier_Transition |
|---------|--------------------------------------|----------------------------|
| **Timing** | Linear only | Bezier curve support |
| **Rotation** | Component-wise Lerp (naive) | Quaternion Slerp (correct) |
| **Extensibility** | Hardcoded in PCM | Modifier-based, reusable |
| **Presets** | None | JSON-based preset library |
| **UI** | No editor | ImGui Bezier editor |
| **Lua Access** | Limited | Full API |

### Future Enhancements

1. **Transition Queuing**: Chain multiple transitions sequentially
2. **Path-based Transitions**: Follow spline curves instead of linear interpolation
3. **Event Callbacks**: OnTransitionStart, OnTransitionComplete, OnTransitionHalfway
4. **Blend Spaces**: Transition between multiple POVs with weights
5. **Camera Shake during Transition**: Apply shake modifier on top of transition
6. **Debug Visualization**: Draw transition path in editor viewport

---

## References

- **Unreal Engine Documentation**: Camera Animation System
  - [Camera Management](https://docs.unrealengine.com/5.0/en-US/camera-management-in-unreal-engine/)
  - [Camera Modifiers](https://docs.unrealengine.com/5.0/en-US/API/Runtime/Engine/Camera/UCameraModifier/)

- **FutureEngine Documentation**:
  - `Document/PlayerCameraManager_Implementation_Plan.md`
  - `Document/CameraShakePresetSystem_Implementation_Plan.md`
  - `Document/BezierCurveEditor_CameraShake_Implementation_Plan.md`

- **Code References**:
  - `Engine/Source/Component/Camera/Public/CameraModifier_CameraShake.h` (Pattern reference)
  - `Engine/Source/Global/Public/BezierCurve.h` (Bezier curve math)
  - `Engine/Source/ImGui/ImGuiBezierEditor.h` (UI component)

---

**End of Document**
