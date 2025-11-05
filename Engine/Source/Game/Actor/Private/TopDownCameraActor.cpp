#include "pch.h"
#include "Game/Actor/Public/TopDownCameraActor.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/SpringArmComponent.h"

IMPLEMENT_CLASS(ATopDownCameraActor, AActor)

ATopDownCameraActor::ATopDownCameraActor()
    : CameraOffset(-5.0f, 0.0f, 25.0f)
    , FixedRotation(FQuaternion::FromEuler(FVector(0.0f, 80.0f, 0.0f)))
{
    SetName("TopDownCameraActor");

    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>();
    SpringArmComponent->SetCanEverTick(true);
    // CameraComponent 생성
    CameraComponent = CreateDefaultSubobject<UCameraComponent>();

    bCanEverTick = true;  // Tick 활성화 (Player 추적용)
}

ATopDownCameraActor::~ATopDownCameraActor()
{
}

UClass* ATopDownCameraActor::GetDefaultRootComponent()
{
    return USceneComponent::StaticClass();
}

void ATopDownCameraActor::InitializeComponents()
{
    AActor::InitializeComponents();

    USceneComponent* RootComp = GetRootComponent();
    if (!RootComp)
    {
        UE_LOG_ERROR("ATopDownCameraActor::InitializeComponents - RootComponent가 null입니다!");
        return;
    }

    // CameraComponent를 루트에 부착
    if (CameraComponent)
    {
        SpringArmComponent->AttachToComponent(RootComp);
        CameraComponent->AttachToComponent(SpringArmComponent);

        // 카메라 파라미터 설정
        CameraComponent->SetFieldOfView(90.0f);
        CameraComponent->SetAspectRatio(16.0f / 9.0f);
        CameraComponent->SetNearClipPlane(1.0f);
        CameraComponent->SetFarClipPlane(10000.0f);
    }
    else
    {
        UE_LOG_ERROR("ATopDownCameraActor::InitializeComponents - CameraComponent가 null입니다!");
    }
}

void ATopDownCameraActor::BeginPlay()
{
    AActor::BeginPlay();

    // PIE World 복제 시 CameraComponent 포인터 갱신
    if (!CameraComponent)
    {
        CameraComponent = Cast<UCameraComponent>(GetComponentByClass(UCameraComponent::StaticClass()));
    }
}

void ATopDownCameraActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);

    // PIE World 복제 시 CameraComponent 포인터 갱신
    if (!CameraComponent)
    {
        CameraComponent = Cast<UCameraComponent>(GetComponentByClass(UCameraComponent::StaticClass()));
    }

    // 카메라 위치와 회전은 Lua의 TopCamera() 함수가 제어
}

void ATopDownCameraActor::SetFollowTarget(AActor* InTarget)
{
    FollowTarget = InTarget;

    if (FollowTarget)
    {
        // 즉시 타겟 위치로 이동
        FVector TargetLocation = FollowTarget->GetActorLocation();
        SetActorLocation(TargetLocation + CameraOffset);
    }
}

void ATopDownCameraActor::SetCameraOffset(const FVector& InOffset)
{
    CameraOffset = InOffset;
}

void ATopDownCameraActor::SetRotation(const FVector& EulerRotation)
{
    // Euler 각도를 Quaternion으로 변환
    FQuaternion Quat = FQuaternion::FromEuler(EulerRotation);
    SetActorRotation(Quat);

    // 고정 회전도 업데이트 (Tick에서 사용)
    FixedRotation = Quat;
}

FVector ATopDownCameraActor::GetRotation() const
{
    // Quaternion을 Euler 각도로 변환
    FQuaternion Quat = GetActorRotation();
    return Quat.ToEuler();
}
