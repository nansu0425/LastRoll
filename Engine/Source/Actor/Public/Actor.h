#pragma once
#include "Core/Public/Object.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Core/Public/NewObject.h"

class UUUIDTextComponent;
/**
 * @brief Level에서 렌더링되는 UObject 클래스
 * UWorld로부터 업데이트 함수가 호출되면 component들을 순회하며 위치, 애니메이션, 상태 처리
 */
UCLASS()

class AActor : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(AActor, UObject)

public:
	AActor();
	AActor(UObject* InOuter);
	virtual ~AActor() override;

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetActorLocation(const FVector& InLocation) const;
	void SetActorRotation(const FQuaternion& InRotation) const;
	void SetActorScale3D(const FVector& InScale) const;
	void SetUniformScale(bool IsUniform);
	virtual UClass* GetDefaultRootComponent();
	virtual void InitializeComponents();

	bool IsUniformScale() const;

	virtual void BeginPlay();
	virtual void EndPlay();
	virtual void Tick(float DeltaTimes);

	// Getter & Setter
	USceneComponent* GetRootComponent() const { return RootComponent; }
	UActorComponent* GetComponentByClass(UClass* ComponentClass) const;
	TArray<UActorComponent*>& GetOwnedComponents()  { return OwnedComponents; }

	void SetRootComponent(USceneComponent* InOwnedComponents) { RootComponent = InOwnedComponents; }

	const FVector& GetActorLocation() const;
	const FQuaternion& GetActorRotation() const;
	const FVector& GetActorScale3D() const;

	template<class T>
	T* CreateDefaultSubobject(const FName& InName = FName::None)
	{
		static_assert(is_base_of_v<UObject, T>, "생성할 클래스는 UObject를 반드시 상속 받아야 합니다");

		// NewObject 대신 직접 생성하여 불필요한 unique name 생성 방지
		T* NewComponent = new T();

		// 이름 설정 (InName이 None이면 클래스 이름 사용)
		if (!InName.IsNone())
		{
			NewComponent->SetName(InName);
		}
		else
		{
			// InName이 없으면 클래스 이름을 기본값으로 사용
			NewComponent->SetName(FName(T::StaticClass()->GetName().ToString().c_str()));
		}

		// Outer 설정
		NewComponent->SetOuter(this);

		// 컴포넌트 등록
		if (NewComponent)
		{
			NewComponent->SetOwner(this);
			OwnedComponents.push_back(NewComponent);
		}

		return NewComponent;
	}
	
	UActorComponent* CreateDefaultSubobject(UClass* Class)
	{
		// NewObject 대신 CreateDefaultObject를 사용하여 불필요한 unique name 생성 방지
		UObject* NewObject = Class->CreateDefaultObject();
		UActorComponent* NewComponent = Cast<UActorComponent>(NewObject);

		if (NewComponent)
		{
			// 클래스 이름을 컴포넌트 이름으로 설정
			NewComponent->SetName(FName(Class->GetName().ToString().c_str()));
			// Outer 설정
			NewComponent->SetOuter(this);
			// 컴포넌트 등록
			NewComponent->SetOwner(this);
			OwnedComponents.push_back(NewComponent);
		}

		return NewComponent;
	}

	/**
	 * @brief 런타임에 이 액터에 새로운 컴포넌트를 생성하고 등록합니다.
	 * @tparam T UActorComponent를 상속받는 컴포넌트 타입
	 * @param InName 컴포넌트의 이름
	 * @return 생성된 컴포넌트를 가리키는 포인터
	 */
	template<class T>
	T* AddComponent()
	{
		static_assert(std::is_base_of_v<UActorComponent, T>, "추가할 클래스는 UActorComponent를 반드시 상속 받아야 합니다");

		T* NewComponent = NewObject<T>(this);

		if (NewComponent)
		{
			RegisterComponent(NewComponent);
		}

		return NewComponent;
	}
	UActorComponent* AddComponent(UClass* InClass);

	void RegisterComponent(UActorComponent* InNewComponent);

	bool RemoveComponent(UActorComponent* InComponentToDelete, bool bShouldDetachChildren = false);

	bool CanTick() const { return bCanEverTick; }
	void SetCanTick(bool InbCanEverTick) { bCanEverTick = InbCanEverTick; }

	bool CanTickInEditor() const { return bTickInEditor; }
	void SetTickInEditor(bool InbTickInEditor) { bTickInEditor = InbTickInEditor; }

	bool IsPendingDestroy() const { return bIsPendingDestroy; }
	void SetIsPendingDestroy(bool bInIsPendingDestroy) { bIsPendingDestroy = bInIsPendingDestroy; }

	bool IsHidden() const { return bHidden; }
	void SetActorHiddenInGame(bool bInHidden);

	bool IsCollisionEnabled() const { return bActorEnableCollision; }
	void SetActorEnableCollision(bool bInActorEnableCollision);

	// Collision & Overlap
	bool IsOverlappingActor(const AActor* Other) const;

protected:
	bool bCanEverTick = false;
	bool bTickInEditor = false;
	bool bBegunPlay = false;
	/** @brief True if the actor is marked for destruction. */  
	bool bIsPendingDestroy = false;
	bool bHidden = false;
	bool bActorEnableCollision = false;

private:
	void UpdateComponentVisibility(bool bInHidden);
	
	USceneComponent* RootComponent = nullptr;
	TArray<UActorComponent*> OwnedComponents;
	
public:
	virtual UObject* Duplicate() override;
	virtual void DuplicateSubObjects(UObject* DuplicatedObject) override;

	virtual UObject* DuplicateForEditor() override;
	virtual void DuplicateSubObjectsForEditor(UObject* DuplicatedObject) override;
};
