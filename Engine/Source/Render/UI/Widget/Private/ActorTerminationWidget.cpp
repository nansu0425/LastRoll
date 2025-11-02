#include "pch.h"
#include "Render/UI/Widget/Public/ActorTerminationWidget.h"
#include "Level/Public/Level.h"
#include "Manager/Input/Public/InputManager.h"

IMPLEMENT_CLASS(UActorTerminationWidget, UWidget)

void UActorTerminationWidget::Initialize()
{
	// Do Nothing Here
}

void UActorTerminationWidget::Update()
{
	// Do Nothing Here
}

void UActorTerminationWidget::RenderWidget()
{
	auto& InputManager = UInputManager::GetInstance();

	// CRITICAL: 현재 활성화된 World(GWorld)에 맞는 선택된 Actor 사용
	// PIE 모드면 PIE World Actor, Editor 모드면 Editor World Actor
	AActor* SelectedActor = GEditor->GetEditorModule()->GetSelectedActorForCurrentWorld();
	if (!SelectedActor)
	{
		return;
	}

	if (InputManager.IsKeyPressed(EKeyInput::Delete))
	{
		// 컴포넌트 선택시 컴포넌트 삭제를 우선
		if (ImGui::IsWindowFocused() && ActorDetailWidget)
		{
			// CRITICAL: 현재 World에 맞는 선택된 Component 사용
			if (UActorComponent* ActorComp = ActorDetailWidget->GetSelectedComponent())
			{
				DeleteSelectedComponent(SelectedActor, ActorComp);
			}
		}
		else
		{
			DeleteSelectedActor(SelectedActor);
		}
	}
}

/**
 * @brief Selected Actor 삭제 함수
 */
void UActorTerminationWidget::DeleteSelectedActor(AActor* InSelectedActor)
{
	UE_LOG("ActorTerminationWidget: 삭제를 위한 Actor Marking 시작");
	if (!InSelectedActor)
	{
		UE_LOG("ActorTerminationWidget: 삭제를 위한 Actor가 선택되지 않았습니다");
		return;
	}

	// 현재 활성화된 World(PIE 모드면 PIE World, Editor 모드면 Editor World)에서 액터 삭제
	if (!GWorld)
	{
		UE_LOG_ERROR("ActorTerminationWidget: GWorld가 유효하지 않습니다");
		return;
	}

	ULevel* CurrentLevel = GWorld->GetLevel();

	if (!CurrentLevel)
	{
		UE_LOG_ERROR("ActorTerminationWidget: No Current Level To Delete Actor From");
		return;
	}

	UE_LOG_INFO("ActorTerminationWidget: 선택된 Actor를 삭제를 위해 마킹 처리: %s",
	       InSelectedActor->GetName() == FName::GetNone() ? "UnNamed" : InSelectedActor->GetName().ToString().data());

	// 지연 삭제를 사용하여 안전하게 다음 틱에서 삭제
	GWorld->DestroyActor(InSelectedActor);
}

void UActorTerminationWidget::DeleteSelectedComponent(AActor* InSelectedActor, UActorComponent* InSelectedComponent)
{
	UE_LOG("ActorTerminationWidget: Component 삭제 시작 - Actor: %s, Component: %s",
		InSelectedActor ? InSelectedActor->GetName().ToString().c_str() : "null",
		InSelectedComponent ? InSelectedComponent->GetName().ToString().c_str() : "null");

	bool bSuccess = InSelectedActor->RemoveComponent(InSelectedComponent, true);
	if (bSuccess)
	{
		UE_LOG_SUCCESS("ActorTerminationWidget: Component 삭제 성공 - %s",
			InSelectedComponent->GetName().ToString().c_str());

		// ActorDetailWidget의 선택 상태 초기화
		ActorDetailWidget->SetSelectedComponent(nullptr);

		// CRITICAL: GEditor의 선택 상태도 초기화
		// PIE 모드에서는 PIESelectedComponent를, Editor 모드에서는 SelectedComponent를 초기화
		if (GEditor->IsPIESessionActive())
		{
			GEditor->GetEditorModule()->SelectPIEComponent(nullptr);
			UE_LOG("ActorTerminationWidget: PIE Component 선택 상태 초기화");
		}
		else
		{
			GEditor->GetEditorModule()->SelectComponent(nullptr);
			UE_LOG("ActorTerminationWidget: Editor Component 선택 상태 초기화");
		}
	}
	else
	{
		UE_LOG_ERROR("ActorTerminationWidget: Component 삭제 실패 - Actor: %s, Component: %s",
			InSelectedActor ? InSelectedActor->GetName().ToString().c_str() : "null",
			InSelectedComponent ? InSelectedComponent->GetName().ToString().c_str() : "null");
	}
}