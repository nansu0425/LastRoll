#pragma once
#include "Actor/Public/Actor.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * @brief Top-down view 카메라 액터
 *
 * Player를 따라가면서 고정된 top-down 시점을 유지하는 독립 카메라 액터.
 * Player에 CameraComponent를 직접 부착하는 대신, 독립적인 Actor로 관리하여
 * Player의 회전에 영향받지 않고 항상 위에서 아래를 바라보는 시점 유지.
 */
UCLASS()
class ATopDownCameraActor : public AActor
{
    GENERATED_BODY()
    DECLARE_CLASS(ATopDownCameraActor, AActor)

public:
    ATopDownCameraActor();
    virtual ~ATopDownCameraActor() override;

    virtual UClass* GetDefaultRootComponent() override;
    virtual void InitializeComponents() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /**
     * @brief 추적할 타겟 액터 설정
     * @param InTarget 추적할 액터 (일반적으로 Player)
     */
    void SetFollowTarget(AActor* InTarget);

    /**
     * @brief 카메라 오프셋 설정 (타겟 기준 상대 위치)
     * @param InOffset 타겟으로부터의 오프셋 (기본값: Vector(-5, 0, 25))
     */
    void SetCameraOffset(const FVector& InOffset);

    /**
     * @brief 카메라 컴포넌트 가져오기
     */
    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

    /**
     * @brief Lua 호환성을 위한 Euler 각도 Rotation 설정
     * @param EulerRotation Euler 각도 (Roll, Pitch, Yaw)
     */
    void SetRotation(const FVector& EulerRotation);

    /**
     * @brief Lua 호환성을 위한 Euler 각도 Rotation 가져오기
     * @return Euler 각도 (Roll, Pitch, Yaw)
     */
    FVector GetRotation() const;

private:
    USpringArmComponent* SpringArmComponent = nullptr;
    UCameraComponent* CameraComponent = nullptr;
    AActor* FollowTarget = nullptr;  // 추적할 타겟 (Player)
    FVector CameraOffset;            // 타겟 기준 오프셋
    FQuaternion FixedRotation;       // 고정된 카메라 회전 (top-down view)
};
