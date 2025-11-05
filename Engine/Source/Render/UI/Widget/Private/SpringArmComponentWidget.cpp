#include "pch.h"
#include "Render/UI/Widget/Public/SpringArmComponentWidget.h"
#include "Component/Public/SpringArmComponent.h"

IMPLEMENT_CLASS(USpringArmComponentWidget, UWidget)

void USpringArmComponentWidget::Update()
{
	ULevel* CurrentLevel = GWorld->GetLevel();
	if (CurrentLevel)
	{
		UActorComponent* NewSelectedComponent = GEditor->GetEditorModule()->GetSelectedComponent();
		if (Component != NewSelectedComponent)
		{
			Component = Cast<USpringArmComponent>(NewSelectedComponent);
		}
	}
}

void USpringArmComponentWidget::RenderWidget()
{
	if (!Component)
	{
		return;
	}

	// 모든 입력 필드를 검은색으로 설정
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	float TargetArmLength = Component->GetTargetArmLength();
	if (ImGui::DragFloat("TargetArmLength", &TargetArmLength))
	{
		Component->SetTargetArmLength(TargetArmLength);
	}
	FVector SocketOffset = Component->GetSocketOffset();
	if (ImGui::DragFloat3("SocketOffset", &SocketOffset.X))
	{
		Component->SetSocketOffset(SocketOffset);
	}
	FVector TargetOffset = Component->GetTargetOffset();
	if (ImGui::DragFloat3("TargetOffset", &TargetOffset.X))
	{
		Component->SetTargetOffset(TargetOffset);
	}
	bool bUsePawnControlRotation = Component->GetbUsePawnControlRotation();
	if (ImGui::Checkbox("UsePawnControlRotation", &bUsePawnControlRotation))
	{
		Component->SetbUsePawnControlRotation(bUsePawnControlRotation);
	}
	bool bLocationLag = Component->GetbLocationLag();
	if (ImGui::Checkbox("LocationLag", &bLocationLag))
	{
		Component->SetbLocationLag(bLocationLag);
	}
	float LocationLagLinearLerpInterpolation = Component->GetLocationLagLinearLerpInterpolation();
	if (ImGui::DragFloat("LocationLagLinearLerpInterpolation", &LocationLagLinearLerpInterpolation, 1.0f, 0.0f))
	{
		Component->SetLocationLagLinearLerpInterpolation(LocationLagLinearLerpInterpolation);
	}
	float LocationLagSpeed = Component->GetLocationLagSpeed();
	if (ImGui::DragFloat("LocationLagSpeed", &LocationLagSpeed, 1.0f, 0.0f))
	{
		Component->SetLocationLagSpeed(LocationLagSpeed);
	}
	bool bRotationLag = Component->GetbRotationLag();
	if (ImGui::Checkbox("RotationLag", &bRotationLag))
	{
		Component->SetbRotationLag(bRotationLag);
	}
	float RotationLagLinearLerpInterpolation = Component->GetRotationLagLinearLerpInterpolation();
	if (ImGui::DragFloat("RotationLagLinearLerpInterpolation", &RotationLagLinearLerpInterpolation, 1.0f, 0.0f))
	{
		Component->SetRotationLagLinearLerpInterpolation(RotationLagLinearLerpInterpolation);
	}
	float RotationLagSpeed = Component->GetRotationLagSpeed();
	if (ImGui::DragFloat("RotationLagSpeed", &RotationLagSpeed, 1.0f, 0.0f))
	{
		Component->SetRotationLagSpeed(RotationLagSpeed);
	}	
	bool bInHeritPitch = Component->GetbInHeritPitch();
	if (ImGui::Checkbox("InHeritPitch", &bInHeritPitch))
	{
		Component->SetbInHeritPitch(bInHeritPitch);
	}
	bool bInHeritYaw = Component->GetbInHeritYaw();
	if (ImGui::Checkbox("InHeritYaw", &bInHeritYaw))
	{
		Component->SetbInHeritYaw(bInHeritYaw);
	}
	bool bInHeritRoll = Component->GetbInHeritRoll();
	if (ImGui::Checkbox("InHeritRoll", &bInHeritRoll))
	{
		Component->SetbInHeritRoll(bInHeritRoll);
	}

	ImGui::PopStyleColor(3);

	ImGui::Separator();

}
