# APlayerCameraManager & UCameraModifier Implementation Plan

**Date**: 2025-11-04
**Author**: Claude
**Status**: Planning Phase
**Target Engine Version**: FutureEngine v0.1

---

## Table of Contents

1. [Overview](#overview)
2. [Current Architecture](#current-architecture)
3. [System Design](#system-design)
4. [Implementation Phases](#implementation-phases)
5. [Class Specifications](#class-specifications)
6. [File Structure](#file-structure)
7. [Integration Points](#integration-points)
8. [Code Snippets](#code-snippets)
9. [Testing Plan](#testing-plan)
10. [Implementation Checklist](#implementation-checklist)

---

## Overview

### Goals

Implement Unreal Engine's camera management system in FutureEngine with minimal features:
- **APlayerCameraManager**: Manages active camera and camera modifier chain
- **UCameraModifier**: Base class for camera post-processing effects
- **UCameraComponent**: Game-side camera component attachable to actors
- **Modifier Chain Pipeline**: Process camera POV through priority-sorted modifiers to generate final View/Projection matrices

### Key Requirements (User-Specified)

```cpp
class APlayerCameraManager : public AActor
{
    FLinearColor FadeColor;
    float FadeAmount;
    FVector2D FadeAlpha;
    float FadeTime;
    float FadeTimeRemaining;

    FName CameraStyle;
    struct FViewTarget ViewTarget;

    TArray<UCameraModifier*> ModifierList;
    // ...
};

class UCameraModifier : public UObject
{
    APlayerCameraManager* CameraOwner;
    float AlphaInTime;
    float AlphaOutTime;
    float Alpha;
    uint32 bDisabled;
    uint8 Priority;
    // ...
};
```

### Design Principles

1. **Non-Invasive Integration**: Preserve existing editor camera system (UCamera)
2. **Game-Only**: Use PlayerCameraManager only in PIE/Game modes
3. **Modifier Chain**: Priority-based modifier processing (same as UE)
4. **Incremental Implementation**: Start with minimal features, extensible structure

---

## Current Architecture

### Camera System (Editor-Centric)

```
[Editor Mode]
UCamera (owned by FViewportClient)
    ↓ Update()
FCameraConstants (View + Projection matrices)
    ↓
URenderer::RenderLevel()
    ↓ Upload to GPU
Constant Buffer (register b1)
    ↓
Vertex Shader (UberLit.hlsl)
```

**Key Files**:
- `Engine/Source/Editor/Public/Camera.h` - UCamera class
- `Engine/Source/Render/Renderer/Private/Renderer.cpp` - Rendering coordinator
- `Engine/Source/Render/UI/Viewport/Public/ViewportClient.h` - Viewport logic
- `Engine/Source/Global/CoreTypes.h` - FCameraConstants struct

### Current Camera Update Flow

```cpp
// In URenderer::Update() (line 875-889)
for (int32 ViewportIndex = 0; ViewportIndex < NumViewports; ++ViewportIndex)
{
    FViewport* Viewport = GetViewport(ViewportIndex);
    UCamera* CurrentCamera = Viewport->GetViewportClient()->GetCamera();

    // 1. Update camera matrices
    CurrentCamera->Update(LocalViewport);

    // 2. Upload to GPU constant buffer
    FRenderResourceFactory::UpdateConstantBufferData(
        ConstantBufferViewProj,
        CurrentCamera->GetFViewProjConstants()
    );

    // 3. Bind to vertex shader
    Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);

    // 4. Render level
    RenderLevel(Viewport, ViewportIndex);
}
```

### FCameraConstants Structure

```cpp
// Engine/Source/Global/CoreTypes.h
struct FCameraConstants
{
    FMatrix View;              // Camera view matrix (world → view space)
    FMatrix Projection;        // Projection matrix (view → clip space)
    FVector ViewWorldLocation; // Camera world position
    float NearClip;            // Near clipping plane distance
    float FarClip;             // Far clipping plane distance
};
```

### Coordinate System

- **World Space**: Z-up, X-forward, Y-right, Left-Handed
- **Camera Space**: X-right, Y-up, Z-forward (DirectX standard)
- **Matrix System**: Row-major storage, Row-vector multiplication (`Vector * Matrix`)

---

## System Design

### Target Architecture (Game Mode)

```
[Game/PIE Mode]
UCameraComponent (attached to Actor)
    ↓ GetCameraView()
FMinimalViewInfo (Location, Rotation, FOV, etc.)
    ↓
APlayerCameraManager::UpdateCamera()
    ├─ UpdateViewTarget() → Base POV
    ├─ UpdateBlending() → ViewTarget transition
    ├─ ApplyCameraModifiers() → Modifier chain
    │   ├─ UCameraModifier (Priority 0)
    │   ├─ UCameraModifier (Priority 50)
    │   └─ UCameraModifier (Priority 100)
    ├─ UpdateFading() → Screen fade
    └─ UpdateCameraConstants() → Convert to matrices
        ↓
FCameraConstants
    ↓
URenderer::RenderLevel() [modified to use CameraManager]
```

### Key Data Structures

#### FMinimalViewInfo (New)

Camera POV information passed through modifier chain.

```cpp
// Add to Engine/Source/Global/CoreTypes.h
struct FMinimalViewInfo
{
    // Transform
    FVector Location;
    FQuaternion Rotation;

    // Projection
    float FOV;                      // Vertical field of view (degrees)
    float AspectRatio;              // Width / Height
    float NearClipPlane;
    float FarClipPlane;
    float OrthoWidth;               // For orthographic projection
    bool bUsePerspectiveProjection; // true: perspective, false: ortho

    // Conversion
    FCameraConstants ToCameraConstants() const;
};
```

#### FViewTarget (New)

View target structure for camera blending.

```cpp
// Add to Engine/Source/Actor/Public/PlayerCameraManager.h
struct FViewTarget
{
    AActor* Target;                        // Target actor
    UCameraComponent* CameraComponent;     // Target's camera component
    FMinimalViewInfo POV;                  // Current POV data
};
```

---

## Implementation Phases

### Phase 1: Foundation (2.5 hours)

**Goal**: Create base classes and structures

- [x] **FMinimalViewInfo struct** (CoreTypes.h)
- [ ] **UCameraComponent** - Game camera component
- [ ] **UCameraModifier** - Base modifier class
- [ ] **APlayerCameraManager** - Camera manager actor

### Phase 2: Renderer Integration (35 minutes)

**Goal**: Connect camera manager to rendering pipeline

- [ ] **UWorld::CameraManager** - Add camera manager field
- [ ] **UWorld::Tick()** - Call CameraManager->UpdateCamera()
- [ ] **URenderer::RenderLevel()** - Use CameraManager in PIE/Game modes

### Phase 3: Example Modifier (30 minutes)

**Goal**: Implement and test modifier system

- [ ] **UCameraModifier_CameraShake** - Simple shake modifier

### Phase 4: Testing & Validation (40 minutes)

**Goal**: Verify system works end-to-end

- [ ] Create test actor with camera component
- [ ] Setup camera manager in PIE world
- [ ] Test modifier chain execution
- [ ] Visual verification

**Total Estimated Time**: ~4 hours

---

## Class Specifications

### 1. UCameraComponent

**File Location**:
- Header: `Engine/Source/Component/Camera/Public/CameraComponent.h`
- Implementation: `Engine/Source/Component/Camera/Private/CameraComponent.cpp`

**Inheritance**: `USceneComponent` → `UCameraComponent`

**Purpose**: Game camera that can be attached to actors (like player pawn, vehicles, etc.)

#### Fields

```cpp
class UCameraComponent : public USceneComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UCameraComponent, USceneComponent)

private:
    // Projection Parameters
    float FieldOfView;                  // Vertical FOV (degrees), default: 90.0f
    float AspectRatio;                  // Width / Height, default: 16.0f/9.0f
    float NearClipPlane;                // Near Z, default: 1.0f
    float FarClipPlane;                 // Far Z, default: 10000.0f

    // Camera Type
    bool bUsePerspectiveProjection;     // true: perspective, false: ortho
    float OrthoWidth;                   // Orthographic width, default: 1000.0f

public:
    UCameraComponent();
    virtual ~UCameraComponent() override;

    // Main API
    void GetCameraView(FMinimalViewInfo& OutPOV) const;

    // Setters
    void SetFieldOfView(float InFOV);
    void SetAspectRatio(float InAspect);
    void SetNearClipPlane(float InNear);
    void SetFarClipPlane(float InFar);
    void SetProjectionType(bool bInUsePerspective);

    // Getters
    float GetFieldOfView() const { return FieldOfView; }
    float GetAspectRatio() const { return AspectRatio; }

    // Serialization
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
```

#### Key Methods

**GetCameraView()** - Build POV from component transform and projection settings

```cpp
void UCameraComponent::GetCameraView(FMinimalViewInfo& OutPOV) const
{
    // Get world transform from SceneComponent hierarchy
    OutPOV.Location = GetWorldLocation();
    OutPOV.Rotation = GetWorldRotation();

    // Copy projection settings
    OutPOV.FOV = FieldOfView;
    OutPOV.AspectRatio = AspectRatio;
    OutPOV.NearClipPlane = NearClipPlane;
    OutPOV.FarClipPlane = FarClipPlane;
    OutPOV.OrthoWidth = OrthoWidth;
    OutPOV.bUsePerspectiveProjection = bUsePerspectiveProjection;
}
```

---

### 2. UCameraModifier

**File Location**:
- Header: `Engine/Source/Component/Camera/Public/CameraModifier.h`
- Implementation: `Engine/Source/Component/Camera/Private/CameraModifier.cpp`

**Inheritance**: `UObject` → `UCameraModifier`

**Purpose**: Base class for camera post-processing effects (shake, lag, FOV transitions, etc.)

#### Enums

```cpp
UENUM()
enum class ECameraModifierBlendMode : uint8
{
    Disabled,      // Modifier inactive
    BlendingIn,    // Alpha increasing (0.0 → 1.0)
    Active,        // Alpha at 1.0 (fully active)
    BlendingOut,   // Alpha decreasing (1.0 → 0.0)
    End
};
DECLARE_UINT8_ENUM_REFLECTION(ECameraModifierBlendMode)
```

#### Fields

```cpp
class UCameraModifier : public UObject
{
    GENERATED_BODY()
    DECLARE_CLASS(UCameraModifier, UObject)

protected:
    // Owner
    APlayerCameraManager* CameraOwner;  // Back-reference to manager

    // Blend Timing
    float AlphaInTime;                  // Time to blend in (0.0 = instant)
    float AlphaOutTime;                 // Time to blend out (0.0 = instant)
    float Alpha;                        // Current blend weight [0.0, 1.0]

    // State
    ECameraModifierBlendMode BlendMode; // Current blend state
    bool bDisabled;                     // If true, skip this modifier
    uint8 Priority;                     // Higher = processed later (stronger influence)

public:
    UCameraModifier();
    virtual ~UCameraModifier() override;

    // Lifecycle (called by CameraManager)
    virtual void Initialize(APlayerCameraManager* InOwner);
    virtual void UpdateAlpha(float DeltaTime);

    // Main Override Point
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV);

    // Control
    virtual void EnableModifier();
    virtual void DisableModifier(bool bImmediate);

    // Getters
    bool IsDisabled() const { return bDisabled; }
    uint8 GetPriority() const { return Priority; }
    float GetAlpha() const { return Alpha; }
    APlayerCameraManager* GetCameraOwner() const { return CameraOwner; }
};
```

#### Key Methods

**UpdateAlpha()** - Handle blend in/out timing

```cpp
void UCameraModifier::UpdateAlpha(float DeltaTime)
{
    if (BlendMode == ECameraModifierBlendMode::Disabled)
    {
        Alpha = 0.0f;
        return;
    }

    if (BlendMode == ECameraModifierBlendMode::BlendingIn)
    {
        if (AlphaInTime > 0.0f)
        {
            Alpha += DeltaTime / AlphaInTime;
            if (Alpha >= 1.0f)
            {
                Alpha = 1.0f;
                BlendMode = ECameraModifierBlendMode::Active;
            }
        }
        else
        {
            Alpha = 1.0f;
            BlendMode = ECameraModifierBlendMode::Active;
        }
    }
    else if (BlendMode == ECameraModifierBlendMode::BlendingOut)
    {
        if (AlphaOutTime > 0.0f)
        {
            Alpha -= DeltaTime / AlphaOutTime;
            if (Alpha <= 0.0f)
            {
                Alpha = 0.0f;
                BlendMode = ECameraModifierBlendMode::Disabled;
            }
        }
        else
        {
            Alpha = 0.0f;
            BlendMode = ECameraModifierBlendMode::Disabled;
        }
    }
}
```

**ModifyCamera()** - Virtual method (override in derived classes)

```cpp
bool UCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    // Base implementation: no modification
    return false;
}
```

---

### 3. APlayerCameraManager

**File Location**:
- Header: `Engine/Source/Actor/Public/PlayerCameraManager.h`
- Implementation: `Engine/Source/Actor/Private/PlayerCameraManager.cpp`

**Inheritance**: `AActor` → `APlayerCameraManager`

**Purpose**: Manages active camera view target and processes camera modifier chain each frame

#### Fields

```cpp
class APlayerCameraManager : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(APlayerCameraManager, AActor)

private:
    // ===== View Target =====
    FViewTarget ViewTarget;             // Current view target
    FViewTarget PendingViewTarget;      // Target to blend to (during transition)

    // ===== Blending =====
    float BlendTime;                    // Total blend duration
    float BlendTimeRemaining;           // Time left to complete blend
    bool bIsBlending;                   // True during view target transition

    // ===== Camera Fading =====
    FLinearColor FadeColor;             // Fade overlay color (e.g., black)
    float FadeAmount;                   // Target fade amount [0.0, 1.0]
    FVector2D FadeAlpha;                // (Current, Target) fade alpha
    float FadeTime;                     // Fade duration
    float FadeTimeRemaining;            // Fade time left
    bool bIsFading;                     // True during fade

    // ===== Camera Style =====
    FName CameraStyle;                  // Current camera style name (e.g., "Default", "ThirdPerson")

    // ===== Camera Modifiers =====
    TArray<UCameraModifier*> ModifierList;  // Active camera modifiers

    // ===== Cached Results =====
    FMinimalViewInfo CachedPOV;         // Final POV after modifier chain
    FCameraConstants CachedCameraConstants; // Final View/Projection matrices

public:
    APlayerCameraManager();
    virtual ~APlayerCameraManager() override;

    // ===== Lifecycle =====
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ===== View Target Management =====
    void SetViewTarget(AActor* NewTarget, float InBlendTime = 0.0f);
    AActor* GetViewTarget() const;
    UCameraComponent* GetViewTargetCamera() const;

    // ===== Camera Modifier Management =====
    UCameraModifier* AddCameraModifier(UClass* ModifierClass);
    bool RemoveCameraModifier(UCameraModifier* Modifier);
    UCameraModifier* FindCameraModifierByClass(UClass* ModifierClass) const;
    void ClearAllCameraModifiers();

    // ===== Camera Fade API =====
    void StartCameraFade(float FromAlpha, float ToAlpha, float Duration, FLinearColor Color);
    void StopCameraFade();

    // ===== Main Update Function =====
    void UpdateCamera(float DeltaTime);

    // ===== Final Camera Access =====
    const FCameraConstants& GetCameraConstants() const { return CachedCameraConstants; }
    const FMinimalViewInfo& GetCameraCachePOV() const { return CachedPOV; }

protected:
    // ===== Internal Update Pipeline =====
    void UpdateViewTarget(float DeltaTime);      // Step 1: Get base POV from ViewTarget
    void UpdateBlending(float DeltaTime);        // Step 2: Blend ViewTargets if transitioning
    void ApplyCameraModifiers(float DeltaTime);  // Step 3: Apply modifier chain
    void UpdateFading(float DeltaTime);          // Step 4: Update screen fade
    void UpdateCameraConstants();                // Step 5: Convert POV to matrices

    // ===== Serialization =====
public:
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
```

#### Update Pipeline (Critical)

```cpp
void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
    // 1. Get base POV from current ViewTarget
    UpdateViewTarget(DeltaTime);

    // 2. Blend between ViewTargets if transitioning
    if (bIsBlending)
    {
        UpdateBlending(DeltaTime);
    }

    // 3. Apply camera modifier chain (priority order)
    ApplyCameraModifiers(DeltaTime);

    // 4. Update screen fade effect
    if (bIsFading)
    {
        UpdateFading(DeltaTime);
    }

    // 5. Convert final POV to View/Projection matrices
    UpdateCameraConstants();
}
```

#### Key Methods Detail

**UpdateViewTarget()** - Get POV from current view target's camera

```cpp
void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
    if (ViewTarget.CameraComponent)
    {
        // Get POV from camera component
        ViewTarget.CameraComponent->GetCameraView(ViewTarget.POV);
    }
    else if (ViewTarget.Target)
    {
        // No camera component: use actor's location/rotation
        ViewTarget.POV.Location = ViewTarget.Target->GetActorLocation();
        ViewTarget.POV.Rotation = ViewTarget.Target->GetActorRotation();
        // Use default FOV, aspect, etc.
    }

    // Store in cached POV
    CachedPOV = ViewTarget.POV;
}
```

**ApplyCameraModifiers()** - Process modifier chain

```cpp
void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime)
{
    // Sort modifiers by priority (ascending: low priority first)
    ModifierList.Sort([](const UCameraModifier& A, const UCameraModifier& B) {
        return A.GetPriority() < B.GetPriority();
    });

    // Apply each modifier in order
    for (UCameraModifier* Modifier : ModifierList)
    {
        if (Modifier && !Modifier->IsDisabled())
        {
            // Update blend alpha
            Modifier->UpdateAlpha(DeltaTime);

            // Apply modifier to POV
            if (Modifier->GetAlpha() > 0.0f)
            {
                Modifier->ModifyCamera(DeltaTime, CachedPOV);
            }
        }
    }
}
```

**UpdateCameraConstants()** - Convert POV to matrices

```cpp
void APlayerCameraManager::UpdateCameraConstants()
{
    // Use FMinimalViewInfo::ToCameraConstants() helper
    CachedCameraConstants = CachedPOV.ToCameraConstants();
}
```

---

### 4. UCameraModifier_CameraShake (Example)

**File Location**:
- Header: `Engine/Source/Component/Camera/Public/CameraModifier_CameraShake.h`
- Implementation: `Engine/Source/Component/Camera/Private/CameraModifier_CameraShake.cpp`

**Inheritance**: `UCameraModifier` → `UCameraModifier_CameraShake`

**Purpose**: Simple sine-wave camera shake effect

#### Fields

```cpp
class UCameraModifier_CameraShake : public UCameraModifier
{
    GENERATED_BODY()
    DECLARE_CLASS(UCameraModifier_CameraShake, UCameraModifier)

private:
    // Shake Parameters
    float ShakeDuration;                // Total shake duration (seconds)
    float ShakeTimeRemaining;           // Time left
    FVector LocationAmplitude;          // Position shake magnitude (world units)
    FVector RotationAmplitude;          // Rotation shake magnitude (degrees)
    float Frequency;                    // Shake frequency (Hz)

    // Internal State
    float ShakeTime;                    // Accumulated time for wave function

public:
    UCameraModifier_CameraShake();
    virtual ~UCameraModifier_CameraShake() override;

    // Control
    void StartShake(float Duration, FVector LocAmp, FVector RotAmp, float Freq);
    void StopShake(bool bImmediate);

    // Override
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
};
```

#### Key Methods

**ModifyCamera()** - Apply shake to camera

```cpp
bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    if (ShakeTimeRemaining <= 0.0f)
    {
        return false;  // No modification
    }

    // Update shake time
    ShakeTimeRemaining -= DeltaTime;
    ShakeTime += DeltaTime;

    // Sine wave shake
    float Wave = sin(ShakeTime * Frequency * 2.0f * PI);

    // Apply location offset (scaled by alpha)
    FVector LocOffset = LocationAmplitude * Wave * Alpha;
    InOutPOV.Location = InOutPOV.Location + LocOffset;

    // Apply rotation offset
    FVector RotOffset = RotationAmplitude * Wave * Alpha;
    FQuaternion RotDelta = FQuaternion::CreateFromYawPitchRoll(
        RotOffset.y * (PI / 180.0f),  // Yaw
        RotOffset.x * (PI / 180.0f),  // Pitch
        RotOffset.z * (PI / 180.0f)   // Roll
    );
    InOutPOV.Rotation = InOutPOV.Rotation * RotDelta;

    return true;  // Modified
}
```

---

## File Structure

### New Files to Create

```
Engine/Source/
├── Component/Camera/              ← NEW DIRECTORY
│   ├── Public/
│   │   ├── CameraComponent.h              [NEW] UCameraComponent
│   │   ├── CameraModifier.h               [NEW] UCameraModifier base
│   │   └── CameraModifier_CameraShake.h   [NEW] Example modifier
│   │
│   └── Private/
│       ├── CameraComponent.cpp            [NEW]
│       ├── CameraModifier.cpp             [NEW]
│       └── CameraModifier_CameraShake.cpp [NEW]
│
└── Actor/
    ├── Public/
    │   └── PlayerCameraManager.h          [NEW] APlayerCameraManager
    │
    └── Private/
        └── PlayerCameraManager.cpp        [NEW]
```

### Files to Modify

```
Engine/Source/
├── Global/
│   └── CoreTypes.h                        [MODIFY] Add FMinimalViewInfo
│
├── Level/
│   ├── Public/World.h                     [MODIFY] Add CameraManager field
│   └── Private/World.cpp                  [MODIFY] Update Tick()
│
├── Render/Renderer/
│   └── Private/Renderer.cpp               [MODIFY] Update RenderLevel()
│
└── Engine.vcxproj                         [MODIFY] Add new files to project
    Engine.vcxproj.filters                 [MODIFY] Add filters for new files
```

---

## Integration Points

### 1. FMinimalViewInfo in CoreTypes.h

**File**: `Engine/Source/Global/CoreTypes.h`

**Location**: Add after `FCameraConstants` struct

```cpp
// Camera POV information (used by camera modifier chain)
struct FMinimalViewInfo
{
    // Transform
    FVector Location;
    FQuaternion Rotation;

    // Projection Parameters
    float FOV;                      // Vertical field of view (degrees)
    float AspectRatio;              // Width / Height
    float NearClipPlane;
    float FarClipPlane;
    float OrthoWidth;               // For orthographic projection
    bool bUsePerspectiveProjection; // true: perspective, false: ortho

    // Default Constructor
    FMinimalViewInfo()
        : Location(FVector::ZeroVector)
        , Rotation(FQuaternion::Identity())
        , FOV(90.0f)
        , AspectRatio(16.0f / 9.0f)
        , NearClipPlane(1.0f)
        , FarClipPlane(10000.0f)
        , OrthoWidth(1000.0f)
        , bUsePerspectiveProjection(true)
    {
    }

    // Convert to FCameraConstants
    FCameraConstants ToCameraConstants() const;
};
```

**ToCameraConstants() Implementation** (in CoreTypes.cpp or inline):

```cpp
FCameraConstants FMinimalViewInfo::ToCameraConstants() const
{
    FCameraConstants Result;

    // Build View matrix (same as UCamera logic)
    FMatrix Translation = FMatrix::CreateTranslation(-Location);

    FVector Forward = Rotation.GetForwardVector();
    FVector Right = Rotation.GetRightVector();
    FVector Up = Rotation.GetUpVector();

    FMatrix Rotation;
    Rotation[0] = FVector4(Right.x, Right.y, Right.z, 0.0f);
    Rotation[1] = FVector4(Up.x, Up.y, Up.z, 0.0f);
    Rotation[2] = FVector4(Forward.x, Forward.y, Forward.z, 0.0f);
    Rotation[3] = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
    Rotation = Rotation.Transpose();

    Result.View = Translation * Rotation;

    // Build Projection matrix
    if (bUsePerspectiveProjection)
    {
        // Perspective projection (manual calculation)
        float FovYRad = FOV * (PI / 180.0f);
        float f = 1.0f / tan(FovYRad / 2.0f);

        Result.Projection = FMatrix::Identity();
        Result.Projection[0][0] = f / AspectRatio;
        Result.Projection[1][1] = f;
        Result.Projection[2][2] = FarClipPlane / (FarClipPlane - NearClipPlane);
        Result.Projection[2][3] = 1.0f;
        Result.Projection[3][2] = (-NearClipPlane * FarClipPlane) / (FarClipPlane - NearClipPlane);
        Result.Projection[3][3] = 0.0f;
    }
    else
    {
        // Orthographic projection
        float Width = OrthoWidth;
        float Height = OrthoWidth / AspectRatio;
        Result.Projection = FMatrix::CreateOrthographicLH(
            -Width / 2.0f, Width / 2.0f,
            -Height / 2.0f, Height / 2.0f,
            NearClipPlane, FarClipPlane
        );
    }

    Result.ViewWorldLocation = Location;
    Result.NearClip = NearClipPlane;
    Result.FarClip = FarClipPlane;

    return Result;
}
```

---

### 2. UWorld Camera Manager Integration

**File**: `Engine/Source/Level/Public/World.h`

**Add Field**:

```cpp
class UWorld : public UObject
{
    DECLARE_CLASS(UWorld, UObject)

private:
    EWorldType WorldType;
    ULevel* Level;
    TArray<AActor*> PendingDestroyActors;
    bool bBegunPlay;

    // Camera Manager (for Game/PIE modes)
    APlayerCameraManager* CameraManager;  // ← ADD THIS

public:
    // ... existing methods ...

    // Camera Manager Access
    APlayerCameraManager* GetCameraManager() const { return CameraManager; }
    void SetCameraManager(APlayerCameraManager* InManager) { CameraManager = InManager; }
};
```

**File**: `Engine/Source/Level/Private/World.cpp`

**Modify Tick()**:

```cpp
void UWorld::Tick(float DeltaTime)
{
    if (!bBegunPlay)
        return;

    // Tick all actors
    const TArray<AActor*>& Actors = Level->GetActors();
    for (AActor* Actor : Actors)
    {
        if (Actor->CanEverTick())
        {
            Actor->Tick(DeltaTime);
        }
    }

    // Update camera manager (Game/PIE only)
    if (CameraManager && (WorldType == EWorldType::Game || WorldType == EWorldType::PIE))
    {
        CameraManager->UpdateCamera(DeltaTime);  // ← ADD THIS
    }

    // Flush pending destroy actors
    for (AActor* Actor : PendingDestroyActors)
    {
        if (Actor)
        {
            Actor->EndPlay();
            delete Actor;
        }
    }
    PendingDestroyActors.clear();
}
```

---

### 3. URenderer Integration

**File**: `Engine/Source/Render/Renderer/Private/Renderer.cpp`

**Modify RenderLevel()** (around line 986):

```cpp
void URenderer::RenderLevel(FViewport* InViewport, int32 ViewportIndex)
{
    // Determine which world to render
    UWorld* WorldToRender = GEditor->GetWorldForViewport(ViewportIndex);
    ULevel* CurrentLevel = WorldToRender->GetLevel();

    // ===== CRITICAL CHANGE: Camera Selection =====
    FCameraConstants ViewProj;
    UCamera* EditorCamera = nullptr;

    if ((WorldToRender->GetWorldType() == EWorldType::Game ||
         WorldToRender->GetWorldType() == EWorldType::PIE) &&
        WorldToRender->GetCameraManager() != nullptr)
    {
        // Use game camera manager
        ViewProj = WorldToRender->GetCameraManager()->GetCameraConstants();

        UE_LOG_DEBUG("Using PlayerCameraManager for rendering (PIE/Game mode)");
    }
    else
    {
        // Use editor camera (existing behavior)
        EditorCamera = InViewport->GetViewportClient()->GetCamera();
        ViewProj = EditorCamera->GetFViewProjConstants();
    }
    // ===== END CHANGE =====

    // Upload to GPU constant buffer
    FRenderResourceFactory::UpdateConstantBufferData(ConstantBufferViewProj, &ViewProj);
    Pipeline->SetConstantBuffer(1, EShaderType::VS, ConstantBufferViewProj);

    // ... rest of rendering pipeline (unchanged) ...

    // Frustum culling
    TArray<UPrimitiveComponent*> FinalVisiblePrims;
    if (EditorCamera)
    {
        // Use editor camera's culling
        FinalVisiblePrims = EditorCamera->GetViewVolumeCuller().GetVisibleComponents();
    }
    else
    {
        // TODO: Implement game camera frustum culling
        // For now, render all components
        FinalVisiblePrims = AllPrimitiveComponents;
    }

    // ... rest of rendering ...
}
```

---

## Code Snippets

### Complete UCameraComponent Implementation Template

```cpp
// ===== CameraComponent.h =====
#pragma once
#include "Component/Public/SceneComponent.h"

struct FMinimalViewInfo; // Forward declaration

UCLASS()
class UCameraComponent : public USceneComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(UCameraComponent, USceneComponent)

private:
    float FieldOfView;
    float AspectRatio;
    float NearClipPlane;
    float FarClipPlane;
    bool bUsePerspectiveProjection;
    float OrthoWidth;

public:
    UCameraComponent();
    virtual ~UCameraComponent() override;

    void GetCameraView(FMinimalViewInfo& OutPOV) const;

    void SetFieldOfView(float InFOV) { FieldOfView = InFOV; }
    void SetAspectRatio(float InAspect) { AspectRatio = InAspect; }
    void SetNearClipPlane(float InNear) { NearClipPlane = InNear; }
    void SetFarClipPlane(float InFar) { FarClipPlane = InFar; }

    float GetFieldOfView() const { return FieldOfView; }
    float GetAspectRatio() const { return AspectRatio; }

    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};

// ===== CameraComponent.cpp =====
#include "pch.h"
#include "CameraComponent.h"
#include "Global/CoreTypes.h"

IMPLEMENT_CLASS(UCameraComponent, USceneComponent)

UCameraComponent::UCameraComponent()
    : FieldOfView(90.0f)
    , AspectRatio(16.0f / 9.0f)
    , NearClipPlane(1.0f)
    , FarClipPlane(10000.0f)
    , bUsePerspectiveProjection(true)
    , OrthoWidth(1000.0f)
{
}

UCameraComponent::~UCameraComponent()
{
}

void UCameraComponent::GetCameraView(FMinimalViewInfo& OutPOV) const
{
    // Get world transform from scene component hierarchy
    OutPOV.Location = GetWorldLocation();
    OutPOV.Rotation = GetWorldRotation();

    // Copy projection settings
    OutPOV.FOV = FieldOfView;
    OutPOV.AspectRatio = AspectRatio;
    OutPOV.NearClipPlane = NearClipPlane;
    OutPOV.FarClipPlane = FarClipPlane;
    OutPOV.OrthoWidth = OrthoWidth;
    OutPOV.bUsePerspectiveProjection = bUsePerspectiveProjection;
}

void UCameraComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        if (InOutHandle.hasKey("FieldOfView"))
            FieldOfView = InOutHandle["FieldOfView"].ToFloat();
        if (InOutHandle.hasKey("AspectRatio"))
            AspectRatio = InOutHandle["AspectRatio"].ToFloat();
        if (InOutHandle.hasKey("NearClipPlane"))
            NearClipPlane = InOutHandle["NearClipPlane"].ToFloat();
        if (InOutHandle.hasKey("FarClipPlane"))
            FarClipPlane = InOutHandle["FarClipPlane"].ToFloat();
        if (InOutHandle.hasKey("bUsePerspectiveProjection"))
            bUsePerspectiveProjection = InOutHandle["bUsePerspectiveProjection"].ToBool();
        if (InOutHandle.hasKey("OrthoWidth"))
            OrthoWidth = InOutHandle["OrthoWidth"].ToFloat();
    }
    else
    {
        InOutHandle["FieldOfView"] = FieldOfView;
        InOutHandle["AspectRatio"] = AspectRatio;
        InOutHandle["NearClipPlane"] = NearClipPlane;
        InOutHandle["FarClipPlane"] = FarClipPlane;
        InOutHandle["bUsePerspectiveProjection"] = bUsePerspectiveProjection;
        InOutHandle["OrthoWidth"] = OrthoWidth;
    }
}
```

---

### APlayerCameraManager::UpdateCamera() Full Implementation

```cpp
void APlayerCameraManager::UpdateCamera(float DeltaTime)
{
    // ===== Step 1: Get base POV from ViewTarget =====
    UpdateViewTarget(DeltaTime);

    // ===== Step 2: Blend between ViewTargets if transitioning =====
    if (bIsBlending)
    {
        UpdateBlending(DeltaTime);
    }

    // ===== Step 3: Apply camera modifier chain =====
    ApplyCameraModifiers(DeltaTime);

    // ===== Step 4: Update screen fade =====
    if (bIsFading)
    {
        UpdateFading(DeltaTime);
    }

    // ===== Step 5: Convert final POV to View/Projection matrices =====
    UpdateCameraConstants();
}

void APlayerCameraManager::UpdateViewTarget(float DeltaTime)
{
    if (ViewTarget.CameraComponent)
    {
        // Get POV from camera component
        ViewTarget.CameraComponent->GetCameraView(ViewTarget.POV);
    }
    else if (ViewTarget.Target)
    {
        // No camera component: use actor transform with default projection
        ViewTarget.POV.Location = ViewTarget.Target->GetActorLocation();
        ViewTarget.POV.Rotation = ViewTarget.Target->GetActorRotation();
        // Keep existing FOV, aspect, etc.
    }
    else
    {
        // No view target: use default camera
        ViewTarget.POV.Location = FVector(0, 0, 500);
        ViewTarget.POV.Rotation = FQuaternion::Identity();
    }

    // Store in cached POV
    CachedPOV = ViewTarget.POV;
}

void APlayerCameraManager::UpdateBlending(float DeltaTime)
{
    BlendTimeRemaining -= DeltaTime;

    if (BlendTimeRemaining <= 0.0f)
    {
        // Blend complete
        ViewTarget = PendingViewTarget;
        bIsBlending = false;
        BlendTimeRemaining = 0.0f;
    }
    else
    {
        // Lerp between current and pending
        float BlendAlpha = 1.0f - (BlendTimeRemaining / BlendTime);

        CachedPOV.Location = Lerp(ViewTarget.POV.Location, PendingViewTarget.POV.Location, BlendAlpha);
        CachedPOV.Rotation = FQuaternion::Slerp(ViewTarget.POV.Rotation, PendingViewTarget.POV.Rotation, BlendAlpha);
        CachedPOV.FOV = Lerp(ViewTarget.POV.FOV, PendingViewTarget.POV.FOV, BlendAlpha);
        // ... lerp other properties ...
    }
}

void APlayerCameraManager::ApplyCameraModifiers(float DeltaTime)
{
    // Sort by priority (low to high)
    ModifierList.Sort([](const UCameraModifier& A, const UCameraModifier& B) {
        return A.GetPriority() < B.GetPriority();
    });

    // Apply each modifier
    for (UCameraModifier* Modifier : ModifierList)
    {
        if (Modifier && !Modifier->IsDisabled())
        {
            // Update blend alpha
            Modifier->UpdateAlpha(DeltaTime);

            // Apply to POV
            if (Modifier->GetAlpha() > 0.0f)
            {
                Modifier->ModifyCamera(DeltaTime, CachedPOV);
            }
        }
    }
}

void APlayerCameraManager::UpdateFading(float DeltaTime)
{
    FadeTimeRemaining -= DeltaTime;

    if (FadeTimeRemaining <= 0.0f)
    {
        FadeAlpha.x = FadeAlpha.y;
        FadeTimeRemaining = 0.0f;
        bIsFading = false;
    }
    else
    {
        float FadeBlendAlpha = 1.0f - (FadeTimeRemaining / FadeTime);
        FadeAlpha.x = Lerp(FadeAmount, FadeAlpha.y, FadeBlendAlpha);
    }

    // TODO: Integrate with URenderer for post-process fade overlay
}

void APlayerCameraManager::UpdateCameraConstants()
{
    CachedCameraConstants = CachedPOV.ToCameraConstants();
}
```

---

## Testing Plan

### Test 1: UCameraComponent Basic Functionality

**Setup**:
1. Create test actor `ATestCameraActor` with `UCameraComponent`
2. Place in level at specific location (e.g., (1000, 0, 500))
3. Set FOV to 60 degrees

**Expected**:
- Component serializes/deserializes correctly
- `GetCameraView()` returns correct Location, Rotation, FOV
- Projection matrices calculated correctly

---

### Test 2: APlayerCameraManager View Target

**Setup**:
1. Spawn `APlayerCameraManager` in PIE world
2. Set world's CameraManager reference
3. Call `SetViewTarget(TestCameraActor)`
4. Enter PIE mode

**Expected**:
- Rendering uses TestCameraActor's camera position/rotation
- FOV change on camera component reflects in viewport
- No crashes or errors in console

---

### Test 3: Camera Modifier Chain

**Setup**:
1. Add `UCameraModifier_CameraShake` to CameraManager
2. Start shake with parameters: Duration=2.0s, LocAmp=(50,50,50), RotAmp=(5,5,5), Freq=10Hz
3. Observe in PIE mode

**Expected**:
- Camera shakes for 2 seconds
- Shake amplitude follows sine wave
- Shake stops after 2 seconds
- No visual artifacts or jitter

---

### Test 4: Multiple Modifiers Priority

**Setup**:
1. Add two modifiers: ShakeA (Priority=10), ShakeB (Priority=100)
2. Start both shakes simultaneously
3. ShakeA: small amplitude, ShakeB: large amplitude

**Expected**:
- Both modifiers execute
- ShakeB (higher priority) processed after ShakeA
- Combined effect visible (not just one or the other)

---

### Test 5: View Target Blending

**Setup**:
1. Have two camera actors: CameraA, CameraB at different positions
2. Set ViewTarget to CameraA
3. Call `SetViewTarget(CameraB, 2.0f)` (2 second blend)

**Expected**:
- Camera smoothly transitions from A to B over 2 seconds
- No sudden jumps or discontinuities
- Rotation interpolates via slerp (smooth arc)

---

### Test 6: Editor vs Game Camera Separation

**Setup**:
1. Open level in Editor mode
2. Move editor camera to position X
3. Place game camera actor at position Y
4. Enter PIE mode

**Expected**:
- Editor mode: renders from position X (editor camera)
- PIE mode: renders from position Y (game camera)
- Exiting PIE returns to position X (editor camera unchanged)

---

## Implementation Checklist

### Phase 1: Foundation

- [ ] **1.1 FMinimalViewInfo struct**
  - [ ] Add struct definition to `CoreTypes.h`
  - [ ] Implement `ToCameraConstants()` method
  - [ ] Test View matrix calculation
  - [ ] Test Projection matrix calculation (perspective & ortho)

- [ ] **1.2 UCameraComponent**
  - [ ] Create header file `CameraComponent.h`
  - [ ] Create implementation file `CameraComponent.cpp`
  - [ ] Implement constructor with default values
  - [ ] Implement `GetCameraView()` method
  - [ ] Implement setters/getters
  - [ ] Implement `Serialize()` method
  - [ ] Add reflection macros (`IMPLEMENT_CLASS`)
  - [ ] Add to `Engine.vcxproj` and `.vcxproj.filters`
  - [ ] Compile and verify no errors

- [ ] **1.3 UCameraModifier base class**
  - [ ] Create `ECameraModifierBlendMode` enum with reflection
  - [ ] Create header file `CameraModifier.h`
  - [ ] Create implementation file `CameraModifier.cpp`
  - [ ] Implement `Initialize()` method
  - [ ] Implement `UpdateAlpha()` method (blend in/out logic)
  - [ ] Implement virtual `ModifyCamera()` (base: no-op)
  - [ ] Implement `EnableModifier()` / `DisableModifier()`
  - [ ] Add reflection macros
  - [ ] Add to project files
  - [ ] Compile and verify

- [ ] **1.4 FViewTarget struct**
  - [ ] Add struct definition to `PlayerCameraManager.h`
  - [ ] Test struct layout

- [ ] **1.5 APlayerCameraManager**
  - [ ] Create header file `PlayerCameraManager.h`
  - [ ] Create implementation file `PlayerCameraManager.cpp`
  - [ ] Implement constructor (initialize default values)
  - [ ] Implement `BeginPlay()` and `Tick()`
  - [ ] Implement `SetViewTarget()` method
  - [ ] Implement `AddCameraModifier()` method
  - [ ] Implement `RemoveCameraModifier()` method
  - [ ] Implement `FindCameraModifierByClass()` method
  - [ ] Implement `UpdateCamera()` pipeline
    - [ ] `UpdateViewTarget()`
    - [ ] `UpdateBlending()`
    - [ ] `ApplyCameraModifiers()`
    - [ ] `UpdateFading()`
    - [ ] `UpdateCameraConstants()`
  - [ ] Implement `StartCameraFade()` / `StopCameraFade()`
  - [ ] Implement `Serialize()` method
  - [ ] Add reflection macros
  - [ ] Add to project files
  - [ ] Compile and verify

### Phase 2: Renderer Integration

- [ ] **2.1 UWorld Camera Manager**
  - [ ] Add `CameraManager` field to `World.h`
  - [ ] Add getter/setter methods
  - [ ] Modify `World::Tick()` to call `CameraManager->UpdateCamera()`
  - [ ] Compile and verify

- [ ] **2.2 URenderer Integration**
  - [ ] Modify `URenderer::RenderLevel()` camera selection logic
  - [ ] Add conditional: use CameraManager in PIE/Game modes
  - [ ] Add debug log for camera source
  - [ ] Handle frustum culling for game camera (temporary: render all)
  - [ ] Compile and verify
  - [ ] Test in Editor mode (should use editor camera)
  - [ ] Test in PIE mode (should use game camera if set)

### Phase 3: Example Modifier

- [ ] **3.1 UCameraModifier_CameraShake**
  - [ ] Create header file `CameraModifier_CameraShake.h`
  - [ ] Create implementation file `CameraModifier_CameraShake.cpp`
  - [ ] Implement constructor
  - [ ] Implement `StartShake()` method
  - [ ] Implement `StopShake()` method
  - [ ] Override `ModifyCamera()` with sine wave logic
  - [ ] Add reflection macros
  - [ ] Add to project files
  - [ ] Compile and verify

### Phase 4: Testing & Validation

- [ ] **4.1 Create Test Actor**
  - [ ] Create `ATestCameraActor` with `UCameraComponent`
  - [ ] Add to level
  - [ ] Configure camera position and FOV

- [ ] **4.2 Setup Camera Manager in PIE**
  - [ ] Spawn `APlayerCameraManager` in PIE world BeginPlay
  - [ ] Call `World->SetCameraManager()`
  - [ ] Set ViewTarget to test actor

- [ ] **4.3 Run Tests**
  - [ ] ✅ Test 1: UCameraComponent basic functionality
  - [ ] ✅ Test 2: APlayerCameraManager view target
  - [ ] ✅ Test 3: Camera modifier chain
  - [ ] ✅ Test 4: Multiple modifiers priority
  - [ ] ✅ Test 5: View target blending
  - [ ] ✅ Test 6: Editor vs game camera separation

- [ ] **4.4 Debug & Fix Issues**
  - [ ] Fix any compilation errors
  - [ ] Fix any runtime crashes
  - [ ] Fix visual artifacts
  - [ ] Verify memory management (no leaks)

### Phase 5: Documentation & Cleanup

- [ ] **5.1 Code Comments**
  - [ ] Add `/** */` doc comments to all public APIs
  - [ ] Add inline comments for complex logic
  - [ ] Document modifier priority system

- [ ] **5.2 Update CLAUDE.md**
  - [ ] Document new camera system architecture
  - [ ] Add usage examples
  - [ ] Update class list

- [ ] **5.3 Commit to Git**
  - [ ] Stage all changes
  - [ ] Write descriptive commit message
  - [ ] Verify git diff

---

## Notes & Considerations

### Memory Management

- **CameraModifiers**: Owned by `APlayerCameraManager`, destroyed with manager
- **CameraComponent**: Owned by actor, destroyed with actor
- **CameraManager**: Owned by `UWorld`, must be destroyed when world is destroyed

### Performance

- **Modifier Chain**: O(N log N) sort per frame (can optimize if needed)
- **Matrix Calculation**: Only once per frame (cached in `FCameraConstants`)
- **Frustum Culling**: Currently uses editor camera's octree; need to implement for game camera

### Future Extensions

- **Additional Modifiers**:
  - `UCameraModifier_CameraLag`: Spring-arm follow camera
  - `UCameraModifier_LookAt`: Aim at target
  - `UCameraModifier_FOVTransition`: Smooth FOV changes
  - `UCameraModifier_ScreenSpaceShake`: 2D screen-space shake

- **Camera Stack**: Multiple cameras with blend weights (like Unity Cinemachine)

- **Camera Animation**: Timeline-based camera paths

- **Camera Collision**: Prevent camera from clipping through walls

### Coordinate System Reminders

- **World Space**: Z-up, X-forward, Y-right, Left-Handed
- **Camera Space**: X-right, Y-up, Z-forward (DirectX standard)
- **Matrix Convention**: Row-major, Row-vector multiplication
- **HLSL**: Always use `row_major` for matrix constant buffers

---

## Quick Reference

### Key Classes

| Class | File | Purpose |
|-------|------|---------|
| `UCameraComponent` | `Component/Camera/Public/CameraComponent.h` | Game camera component |
| `UCameraModifier` | `Component/Camera/Public/CameraModifier.h` | Base modifier class |
| `UCameraModifier_CameraShake` | `Component/Camera/Public/CameraModifier_CameraShake.h` | Example shake modifier |
| `APlayerCameraManager` | `Actor/Public/PlayerCameraManager.h` | Camera manager actor |
| `FMinimalViewInfo` | `Global/CoreTypes.h` | Camera POV struct |
| `FViewTarget` | `Actor/Public/PlayerCameraManager.h` | View target struct |

### Key Enums

| Enum | Values | Purpose |
|------|--------|---------|
| `ECameraModifierBlendMode` | Disabled, BlendingIn, Active, BlendingOut | Modifier blend state |
| `EWorldType` | Editor, EditorPreview, PIE, Game | World type (for camera selection) |

### Key Methods

| Method | Class | Purpose |
|--------|-------|---------|
| `GetCameraView()` | `UCameraComponent` | Get POV from camera |
| `UpdateCamera()` | `APlayerCameraManager` | Main update pipeline |
| `ModifyCamera()` | `UCameraModifier` | Override to apply effect |
| `ToCameraConstants()` | `FMinimalViewInfo` | Convert POV to matrices |
| `SetViewTarget()` | `APlayerCameraManager` | Change active camera |
| `AddCameraModifier()` | `APlayerCameraManager` | Add modifier to chain |

---

## Revision History

| Date | Author | Change |
|------|--------|--------|
| 2025-11-04 | Claude | Initial document creation |

---

**End of Document**
