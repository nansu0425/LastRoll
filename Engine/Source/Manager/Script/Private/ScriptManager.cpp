#include "pch.h"
#include "Manager/Script/Public/ScriptManager.h"


// 엔진 인클루드
#include <random>

#include "Global/Vector.h"
#include "Global/Quaternion.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Camera/Public/CameraComponent.h"
#include "Component/Public/ActorComponent.h"
#include "Component/Public/AudioComponent.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Shape/Public/ShapeComponent.h"
#include "Component/Shape/Public/SphereComponent.h"
#include "Component/Shape/Public/BoxComponent.h"
#include "Component/Shape/Public/CapsuleComponent.h"
#include "Physics/Public/CollisionTypes.h"
#include "Manager/Time/Public/TimeManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/UI/Public/ViewportManager.h"
#include "Render/UI/Viewport/Public/Viewport.h"
#include "Render/UI/Viewport/Public/ViewportClient.h"
#include "Source/Editor/Public/Camera.h"
#include "Render/UI/GameUI/Public/GameUI.h"
#include "Editor/Public/Editor.h"
#include "Level/Public/Level.h"
#include "Level/Public/World.h"
#include "Game/Actor/Public/Player.h"
#include "Game/Actor/Public/TopDownCameraActor.h"
IMPLEMENT_SINGLETON_CLASS(UScriptManager, UObject)

UScriptManager::UScriptManager()
	: LuaState(nullptr)
{
}

UScriptManager::~UScriptManager()
{
	Shutdown();
}

void UScriptManager::Initialize()
{
	UE_LOG_SYSTEM("Lua 매니저 초기화 중 (Sol2 사용)...");

	try
	{
		// Lua 상태 생성
		LuaState = new sol::state();

		// 표준 Lua 라이브러리 로드
		LuaState->open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::coroutine,
			sol::lib::string,
			sol::lib::math,
			sol::lib::table,
			sol::lib::debug,
			sol::lib::io
		);

		// 엔진 타입 및 함수 등록
		RegisterCoreTypes();
		RegisterGlobalFunctions();

		UE_LOG_SUCCESS("Lua 매니저 초기화 완료");
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("Lua 매니저 초기화 실패: %s", e.what());
	}
}

void UScriptManager::Shutdown()
{
	if (LuaState)
	{
		// ✅ CRITICAL: LuaState를 삭제하기 전에 모든 sol::table을 먼저 해제
		// 그렇지 않으면 sol::table 소멸자가 이미 삭제된 LuaState에 접근하여 Access Violation 발생
		LuaScriptMap.clear();

		delete LuaState;
		LuaState = nullptr;
		UE_LOG_SYSTEM("Lua 매니저 종료");
	}
}

sol::state& UScriptManager::GetLuaState()
{
	if (!LuaState)
	{
		UE_LOG_ERROR("Lua 상태가 null입니다! Initialize()를 호출했나요?");
		static sol::state dummy;
		return dummy;
	}
	return *LuaState;
}
sol::table UScriptManager::GetTable(const FString& ScriptPath)
{
	// CRITICAL: 각 ScriptComponent가 독립적인 environment를 가지도록
	// 매번 새로운 environment를 생성하여 스크립트를 재실행
	// (캐싱된 GlobalTable을 공유하면 set_on()이 마지막 호출만 적용됨)

	// 스크립트가 로드되지 않았으면 먼저 로드 (Hot Reload 등록용)
	if (!IsLoadedScript(ScriptPath))
	{
		LoadLuaScript(ScriptPath);
	}

	// ✅ 새 정책: Build/[Config]/Data/Scripts만 사용
	UPathManager& PathMgr = UPathManager::GetInstance();
	path BuildScriptPath = PathMgr.GetDataPath() / "Scripts" / ScriptPath.c_str();

	// Build 경로에 없으면 Engine에서 자동 복사 시도
	if (!std::filesystem::exists(BuildScriptPath))
	{
		UE_LOG_INFO("GetTable: Build 경로에 스크립트 없음. Engine에서 복사 시도: %s", ScriptPath.c_str());
		if (!CopyScriptFromEngineToBuild(ScriptPath))
		{
			UE_LOG_WARNING("ScriptManager: 스크립트 파일을 찾을 수 없음: %s", ScriptPath.c_str());
			return LuaState->create_table();
		}
	}

	try
	{
		// 매번 새로운 environment 생성
		sol::environment new_env(*LuaState, sol::create, LuaState->globals());

		// 스크립트를 Build 경로에서 실행
		LuaState->script_file(BuildScriptPath.string(), new_env);

		return new_env;
	}
	catch (const sol::error& e)
	{
		UE_LOG_ERROR("Lua script execution error in GetTable: %s", e.what());
		return LuaState->create_table();
	}
}
bool UScriptManager::IsLoadedScript(const FString& ScriptPath)
{
	auto it = LuaScriptMap.find(ScriptPath);
	if (it != LuaScriptMap.end())
	{
		return true;
	}
	return false;
}

bool UScriptManager::LoadLuaScript(const FString& ScriptPath)
{
	// ✅ 새 정책: Build/[Config]/Data/Scripts만 사용
	UPathManager& PathMgr = UPathManager::GetInstance();
	path BuildScriptPath = PathMgr.GetDataPath() / "Scripts" / ScriptPath.c_str();

	// Build 경로에 없으면 Engine에서 자동 복사 시도
	if (!std::filesystem::exists(BuildScriptPath))
	{
		UE_LOG_INFO("LoadLuaScript: Build 경로에 스크립트 없음. Engine에서 복사 시도: %s", ScriptPath.c_str());
		if (!CopyScriptFromEngineToBuild(ScriptPath))
		{
			UE_LOG_WARNING("ScriptManager: 스크립트 파일을 찾을 수 없어 Hot Reload 등록 실패: %s", ScriptPath.c_str());
			return false;
		}
	}

	std::error_code ErrorCode;
	auto LastWriteTime = std::filesystem::last_write_time(BuildScriptPath, ErrorCode);
	if (!ErrorCode)
	{
		UE_LOG_INFO("루아 스크립트 로드 시작 - %s (경로: Build/Data/Scripts)", ScriptPath.c_str());

		bool bCompileSuccess = false;
		sol::environment env(*LuaState, sol::create, LuaState->globals());

		try
		{
			// lua globals를 fallback으로 하는 environment 생성
			// 스크립트의 함수 정의들은 env에 저장되고, 찾지 못한 것은 globals에서 찾음

			// environment 내에서 스크립트 실행
			LuaState->script_file(BuildScriptPath.string(), env);

			bCompileSuccess = true;
		}
		catch (const sol::error& e)
		{
			UE_LOG_ERROR("Lua compile/load error: %s", e.what());
			// ✅ 에러가 발생해도 Hot Reload 등록은 계속 진행 (빈 GlobalTable로)
			// 나중에 수정하면 Hot Reload로 재시도 가능
			bCompileSuccess = false;
		}

		// ✅ CRITICAL: 컴파일 성공 여부와 관계없이 LuaScriptMap에 등록
		// 컴파일 에러가 발생해도 등록해두면, 나중에 수정했을 때 Hot Reload로 다시 로드 시도
		LuaScriptMap[ScriptPath].Path = ScriptPath;
		LuaScriptMap[ScriptPath].LastCompileTime = LastWriteTime;
		LuaScriptMap[ScriptPath].GlobalTable = bCompileSuccess ? env : LuaState->create_table();

		if (bCompileSuccess)
		{
			UE_LOG_SUCCESS("ScriptManager: Hot Reload 등록 완료 - %s", ScriptPath.c_str());
		}
		else
		{
			UE_LOG_WARNING("ScriptManager: 컴파일 에러 발생했지만 Hot Reload는 등록됨 - %s (수정 후 자동 재로드됨)", ScriptPath.c_str());
		}

		return bCompileSuccess;
	}
	else
	{
		UE_LOG_WARNING("ScriptManager: 스크립트 수정 시간 조회 실패: %s (%s)", ScriptPath.c_str(), ErrorCode.message().c_str());
		return false;
	}
}

bool UScriptManager::ExecuteFile(const FString& FilePath)
{
	if (!LuaState)
	{
		UE_LOG_ERROR("파일 실행 불가: Lua 상태가 null입니다");
		return false;
	}

	try
	{
		sol::protected_function_result result = LuaState->script_file(FilePath.c_str());

		if (!result.valid())
		{
			sol::error err = result;
			UE_LOG_ERROR("%s 스크립트 오류: %s", FilePath.c_str(), err.what());
			return false;
		}

		UE_LOG_INFO("Lua 파일 실행 완료: %s", FilePath.c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("Lua 파일 실행 중 예외 발생 %s: %s", FilePath.c_str(), e.what());
		return false;
	}
}

bool UScriptManager::ExecuteString(const FString& Code)
{
	if (!LuaState)
	{
		UE_LOG_ERROR("문자열 실행 불가: Lua 상태가 null입니다");
		return false;
	}

	try
	{
		sol::protected_function_result result = LuaState->script(Code.c_str());

		if (!result.valid())
		{
			sol::error err = result;
			UE_LOG_ERROR("Lua 코드 오류: %s", err.what());
			return false;
		}

		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("Lua 코드 실행 중 예외 발생: %s", e.what());
		return false;
	}
}

void UScriptManager::RegisterCoreTypes()
{
	sol::state& lua = *LuaState;

	// ====================================================================
	// FVector - 연산자 오버로딩이 있는 3D 벡터
	// ====================================================================

	// Vector 생성자 함수 등록 (호출 가능하게)
	lua.set_function("Vector", sol::overload(
		[]() { return FVector(0.0f, 0.0f, 0.0f); },
		[](float x, float y, float z) { return FVector(x, y, z); }
	));

	lua.set_function("Vector2", sol::overload(
		[]() { return FVector2(0.0f, 0.0f); },
		[](float x, float y) { return FVector2(x, y); }
	));


	lua.set_function("Vector4", sol::overload(
		[]() { return FVector4(0.0f, 0.0f, 0.0f, 0.0f); },
		[](float x, float y, float z, float w) { return FVector4(x, y, z, w); }
	));

	lua.new_usertype<FVector2>("FVector2",
		sol::no_constructor,  // 생성자는 위에서 Vector 함수로 등록했음

		// Properties
		"x", &FVector2::X,
		"y", &FVector2::Y,

		// Operators
		sol::meta_function::addition, [](const FVector2& a, const FVector2& b) -> FVector2 {
			return FVector2(a.X + b.X, a.Y + b.Y);
		},
		sol::meta_function::subtraction, [](const FVector2& a, const FVector2& b) -> FVector2 {
			return FVector2(a.X - b.X, a.Y - b.Y);
		},
		sol::meta_function::multiplication, sol::overload(
			[](const FVector2& v, float f) -> FVector2 { return v * f; },
			[](float f, const FVector2& v) -> FVector2 { return v * f; }
		),
		"Length", & FVector2::Length,
		"Normalize", & FVector2::GetNormalized
	);

	lua.new_usertype<FVector4>("FVector4",
		sol::no_constructor,  // 생성자는 위에서 Vector 함수로 등록했음

		// Properties
		"x", &FVector4::X,
		"y", &FVector4::Y,
		"z", &FVector4::Z,
		"w", &FVector4::W,

		// Operators
		sol::meta_function::addition, [](const FVector4& a, const FVector4& b) -> FVector4 {
			return a + b;
		},
		sol::meta_function::subtraction, [](const FVector4& a, const FVector4& b) -> FVector4 {
			return a - b;
		},
		sol::meta_function::multiplication, sol::overload(
			[](const FVector4& v, float f) -> FVector4 { return v * f; },
			[](float f, const FVector4& v) -> FVector4 { return v * f; }
		)
	);


	// FVector usertype 등록 (메서드와 프로퍼티)
	lua.new_usertype<FVector>("FVector",
		sol::no_constructor,  // 생성자는 위에서 Vector 함수로 등록했음

		// Properties
		"x", &FVector::X,
		"y", &FVector::Y,
		"z", &FVector::Z,

		// Operators
		sol::meta_function::addition, [](const FVector& a, const FVector& b) -> FVector {
			return FVector(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
		},
		sol::meta_function::subtraction, [](const FVector& a, const FVector& b) -> FVector {
			return FVector(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
		},
		sol::meta_function::multiplication, sol::overload(
			[](const FVector& v, float f) -> FVector { return v * f; },
			[](float f, const FVector& v) -> FVector { return v * f; }
		),

		// Methods
		"Length", &FVector::Length,
		"Normalize", &FVector::Normalize,
		"Dot", [](const FVector& a, const FVector& b) { return a.Dot(b); },
		"Cross", [](const FVector& a, const FVector& b) { return a.Cross(b); }
	);


	// ====================================================================
	// FQuaternion - Rotation representation
	// ====================================================================

	// Quaternion 생성자 함수 등록 (호출 가능하게)
	lua.set_function("Quaternion", sol::overload(
		[]() { return FQuaternion(); },
		[](float x, float y, float z, float w) { return FQuaternion(x, y, z, w); }
	));

	// FQuaternion usertype 등록 (메서드와 프로퍼티)
	lua.new_usertype<FQuaternion>("FQuaternion",
		sol::no_constructor,  // 생성자는 위에서 Quaternion 함수로 등록했음
		"x", &FQuaternion::X,
		"y", &FQuaternion::Y,
		"z", &FQuaternion::Z,
		"w", &FQuaternion::W
	);

	// ====================================================================
	// AActor - Base actor class
	// ====================================================================
	lua.new_usertype<AActor>("Actor",
		// Properties
		"UUID", sol::property(&AActor::GetUUID),
		"Location", sol::property(&AActor::GetActorLocation, &AActor::SetActorLocation),
		"Rotation", sol::property(&AActor::GetActorRotation, &AActor::SetActorRotation),

		// Methods
		"GetName", [](AActor* self) { return self->GetName().ToString(); },
		"GetLocation", &AActor::GetActorLocation,
		"SetLocation", &AActor::SetActorLocation,
		"GetRotation", &AActor::GetActorRotation,
		"SetRotation", &AActor::SetActorRotation,
		"AxisRotation", &AActor::AxisAngle,

		"PrintLocation", [](AActor* self) {
			FVector loc = self->GetActorLocation();
			UE_LOG("Actor %s Location: (%.2f, %.2f, %.2f)",
				self->GetName().ToString().c_str(), loc.X, loc.Y, loc.Z);
		},
		"SetCanTick", &AActor::SetCanTick,
		"SetActorHiddenInGame", &AActor::SetActorHiddenInGame,
		"SetActorEnableCollision", &AActor::SetActorEnableCollision,
		"StopAllCoroutine", &AActor::StopAllCoroutine,

		// Component access - 실제 타입을 반환하도록 다형성 지원
		"GetComponent", [](sol::this_state s, AActor* self, const std::string& ClassName) -> sol::object {
			sol::state_view lua(s);

			UClass* ComponentClass = UClass::FindClass(FName(ClassName.c_str()));
			if (!ComponentClass)
			{
				UE_LOG_WARNING("GetComponent: Class '%s' not found", ClassName.c_str());
				return sol::nil;
			}

			UActorComponent* Component = self->GetComponentByClass(ComponentClass);
			if (!Component)
			{
				return sol::nil;
			}

			// 실제 타입에 따라 적절한 sol::object 생성 (가장 구체적인 타입부터 체크)
			if (auto* SphereComp = Cast<USphereComponent>(Component))
			{
				return sol::make_object(lua, SphereComp);
			}
			else if (auto* BoxComp = Cast<UBoxComponent>(Component))
			{
				return sol::make_object(lua, BoxComp);
			}
			else if (auto* CapsuleComp = Cast<UCapsuleComponent>(Component))
			{
				return sol::make_object(lua, CapsuleComp);
			}
			else if (auto* ShapeComp = Cast<UShapeComponent>(Component))
			{
				return sol::make_object(lua, ShapeComp);
			}
			else if (auto* PrimComp = Cast<UPrimitiveComponent>(Component))
			{
				return sol::make_object(lua, PrimComp);
			}
			else if (auto* ScriptComp = Cast<UScriptComponent>(Component))
			{
				return sol::make_object(lua, ScriptComp);
			}
			else if (auto* SceneComp = Cast<USceneComponent>(Component))
			{
				return sol::make_object(lua, SceneComp);
			}

			// 기본적으로 UActorComponent로 반환
			return sol::make_object(lua, Component);
		},

		// ScriptComponent 전용 접근 메서드 (타입 안전)
		"GetScriptComponent", [](AActor* self) -> UScriptComponent* {
			UActorComponent* Component = self->GetComponentByClass(UScriptComponent::StaticClass());
			return Cast<UScriptComponent>(Component);
		},

		// ScriptComponent 전용 접근 메서드 (타입 안전)
		"GetScriptComponentByName", [](AActor* self, const std::string& ScriptName) -> UScriptComponent* {
			for (UActorComponent* ActorComp : self->GetOwnedComponents())
			{
				if(UScriptComponent* ScriptComp = Cast<UScriptComponent>(ActorComp))
				{
					if (ScriptComp->GetScriptPath() == ScriptName)
					{
						return ScriptComp;
					}
				}
			}
			return nullptr;
		}
	);

	// ====================================================================
	// FOverlapInfo - Overlap 정보 구조체
	// ====================================================================
	lua.new_usertype<FOverlapInfo>("FOverlapInfo",
		sol::no_constructor,  // Lua에서 직접 생성 불가 (C++에서만)

		// Properties
		"OverlappingComponent", sol::property(
			[](const FOverlapInfo& self) -> UPrimitiveComponent* {
				return self.OverlappingComponent;
			}
		),

		// Helper Methods
		"GetOtherActor", [](const FOverlapInfo& self) -> AActor* {
			return self.OverlappingComponent ?
			       self.OverlappingComponent->GetOwner() : nullptr;
		},

		"GetOtherComponent", [](const FOverlapInfo& self) -> UPrimitiveComponent* {
			return self.OverlappingComponent;
		}
	);

	// ====================================================================
	// UPrimitiveComponent - 충돌/렌더링 가능한 컴포넌트
	// ====================================================================
	lua.new_usertype<UPrimitiveComponent>("PrimitiveComponent",
		sol::no_constructor,

		// Collision properties
		"GenerateOverlapEvents", sol::property(
			&UPrimitiveComponent::GetGenerateOverlapEvents,
			&UPrimitiveComponent::SetGenerateOverlapEvents
		),

		"BlockComponent", sol::property(
			&UPrimitiveComponent::GetBlockComponent,
			&UPrimitiveComponent::SetBlockComponent
		),

		// Overlap query methods
		"IsOverlappingActor", &UPrimitiveComponent::IsOverlappingActor,
		"IsOverlappingComponent", &UPrimitiveComponent::IsOverlappingComponent,

		// Inherited from UActorComponent
		"GetOwner", &UPrimitiveComponent::GetOwner,

		"BindBeginOverlap",
		[] (UPrimitiveComponent* PrimitiveComponent, UScriptComponent* ScriptComponent, sol::function LuaFunc) 
		{
			if (!PrimitiveComponent || !ScriptComponent)
			{
				UE_LOG_ERROR("BindBeginOverlap: 유효하지 않은 컴포넌트가 전달되었습니다.");
				return;
			}

			if (!LuaFunc.valid())
			{
				UE_LOG_ERROR("BindBeginOverlap: 유효하지 않은 Lua 함수가 전달되었습니다.");
				return;
			}

			auto WrapperLambda = [LuaFuncCaptured = std::move(LuaFunc)] (const FOverlapInfo& Info)
			{
				if (!LuaFuncCaptured.valid())
				{
					return;
				}

				AActor* OtherActor = Info.OverlappingComponent ?
									 Info.OverlappingComponent->GetOwner() : nullptr;

				sol::protected_function_result  Result = LuaFuncCaptured(OtherActor);
				if (!Result.valid())
				{
					sol::error Error = Result;
					UE_LOG_ERROR("Lua [BindBeginOverlap] Error: %s", Error.what());
				}
			};

			PrimitiveComponent->OnComponentBeginOverlap.AddWeakLambda(
				ScriptComponent,
				std::move(WrapperLambda)
			);

			PrimitiveComponent->OnComponentBeginOverlap.AddLambda(
				[](const FOverlapInfo& Info) { 
					//UE_LOG("Hello World!"); 
				}
			);

			UE_LOG_DEBUG("BindBeginOverlap: Lua 함수가 C++ 델리게이트에 바인딩되었습니다.");
		},

		"BindEndOverlap",
		[] (UPrimitiveComponent* PrimitiveComponent, UScriptComponent* ScriptComponent, sol::function LuaFunc)
		{
			if (!PrimitiveComponent || !ScriptComponent)
			{
				UE_LOG_ERROR("BindEndOverlap: 유효하지 않은 컴포넌트가 전달되었습니다.");
				return;
			}

			if (!LuaFunc.valid())
			{
				UE_LOG_ERROR("BindEndOverlap: 유효하지 않은 Lua 함수가 전달되었습니다.");
				return;
			}

			auto WrapperLambda = [LuaFuncCaptured = std::move(LuaFunc)] (const FOverlapInfo& Info)
			{
				if (!LuaFuncCaptured.valid())
				{
					return;
				}

				AActor* OtherActor = Info.OverlappingComponent ?
									 Info.OverlappingComponent->GetOwner() : nullptr;

				sol::protected_function_result  Result = LuaFuncCaptured(OtherActor);
				if (!Result.valid())
				{
					sol::error Error = Result;
					UE_LOG_ERROR("Lua [BindEndOverlap] Error: %s", Error.what());
				}
			};

			PrimitiveComponent->OnComponentEndOverlap.AddWeakLambda(
				ScriptComponent,
				std::move(WrapperLambda)
			);

			UE_LOG_DEBUG("BindEndOverlap: Lua 함수가 C++ 델리게이트에 바인딩되었습니다.");
		}
	);

	// ====================================================================
	// UShapeComponent - Shape 컴포넌트 기본 클래스
	// ====================================================================
	lua.new_usertype<UShapeComponent>("ShapeComponent",
		sol::no_constructor,

		// 상속 관계 명시 (UPrimitiveComponent 상속)
		sol::base_classes, sol::bases<UPrimitiveComponent>(),

		// Shape color
		"ShapeColor", sol::property(
			&UShapeComponent::GetShapeColor,
			&UShapeComponent::SetShapeColor
		),

		// Draw settings
		"DrawOnlyIfSelected", sol::property(
			&UShapeComponent::IsDrawOnlyIfSelected,
			&UShapeComponent::SetDrawOnlyIfSelected
		)
	);

	// ====================================================================
	// USphereComponent - Sphere 충돌 형상 컴포넌트
	// ====================================================================
	lua.new_usertype<USphereComponent>("USphereComponent",
		sol::no_constructor,

		// 상속 관계 명시 (UShapeComponent 상속)
		sol::base_classes, sol::bases<UShapeComponent, UPrimitiveComponent>(),

		// Sphere radius
		"SphereRadius", sol::property(
			&USphereComponent::GetSphereRadius,
			&USphereComponent::SetSphereRadius
		)
	);

	// ====================================================================
	// UBoxComponent - Box 충돌 형상 컴포넌트
	// ====================================================================
	lua.new_usertype<UBoxComponent>("UBoxComponent",
		sol::no_constructor,

		// 상속 관계 명시 (UShapeComponent 상속)
		sol::base_classes, sol::bases<UShapeComponent, UPrimitiveComponent>()
	);

	// ====================================================================
	// UCapsuleComponent - Capsule 충돌 형상 컴포넌트
	// ====================================================================
	lua.new_usertype<UCapsuleComponent>("UCapsuleComponent",
		sol::no_constructor,

		// 상속 관계 명시 (UShapeComponent 상속)
		sol::base_classes, sol::bases<UShapeComponent, UPrimitiveComponent>()
	);

	// ====================================================================
	// UActorComponent - 컴포넌트 기본 클래스
	// ====================================================================
	lua.new_usertype<UActorComponent>("ActorComponent",
		sol::no_constructor,

		// Methods
		"GetOwner", &UActorComponent::GetOwner
	);

	// ====================================================================
	// UScriptComponent - Lua 스크립팅 컴포넌트
	// ====================================================================
	lua.new_usertype<UScriptComponent>("ScriptComponent",
		sol::no_constructor,

		// 상속 관계 명시 (UActorComponent 상속)
		sol::base_classes, sol::bases<UActorComponent>(),

		// Methods
		"GetEnv", &UScriptComponent::GetEnv,
		"SetScriptPath", &UScriptComponent::SetScriptPath,
		"BeginPlay", &UScriptComponent::BeginPlay
	);
	
	// ====================================================================
	// UAudioComponent - 오디오 컴포넌트
	// ====================================================================
	lua.new_usertype<UAudioComponent>("AudioComponent",
		sol::no_constructor,
		sol::base_classes, sol::bases<UActorComponent>(),

		"Play", &UAudioComponent::Play,
		"Stop", &UAudioComponent::Stop
	);

	// ====================================================================
	// Coroutine
	// ====================================================================

	lua.new_usertype<FWaitCondition>("WaitCondition",
		sol::no_constructor,

		"WaitTime", & FWaitCondition::WaitTime,
		"WaitType", & FWaitCondition::WaitType
	);
	sol::state& LuaState = UScriptManager::GetInstance().GetLuaState();
	LuaState.set_function("WaitForSeconds", sol::overload(
		[](float InWaitTime) { return FWaitCondition(InWaitTime); }
	));

	LuaState.set_function("WaitUntil",
		[](sol::function luaFunc) {
			return FWaitCondition([luaFunc]() -> bool {
				auto result = luaFunc();
				return result.valid() && result.get<bool>();
				});
		}
	);
	LuaState.set_function("WaitTick",
		[]() {
			return FWaitCondition();
		}
	);
	
	// ====================================================================
	// World
	// ====================================================================

	LuaState["SpawnActor"] = []() -> AActor*
	{
		return GWorld->SpawnActor(AActor::StaticClass());
	};

	LuaState["SpawnActorByName"] = [](const FString& ActorName) -> AActor*
	{
		UClass* ActorClass = UClass::FindClass(ActorName);
		if (ActorClass == nullptr)
		{
			UE_LOG_ERROR("SpawnActorByName: 액터 '%s'를 찾을 수 없습니다.", ActorName.c_str());
			return nullptr;
		}
		return GWorld->SpawnActor(ActorClass);
	};

	LuaState["SpawnActorFromScript"] = [](const FString& ScriptName) -> AActor*
	{
		AActor* Actor = GWorld->SpawnActor(AStaticMeshActor::StaticClass());
		UScriptComponent* ScriptComponent = static_cast<UScriptComponent*>(Actor->AddComponent(UScriptComponent::StaticClass()));
		ScriptComponent->SetScriptPath(ScriptName);
		ScriptComponent->BeginPlay();
		return Actor;
	};

	LuaState["SpawnActorByNameFromScript"] = [](const FString& ActorName, const FString& ScriptName) -> AActor*
	{
		UClass* ActorClass = UClass::FindClass(ActorName);
		if (ActorClass == nullptr)
		{
			UE_LOG_ERROR("SpawnActorByName: 액터 '%s'를 찾을 수 없습니다.", ActorName.c_str());
			return nullptr;
		}
		AActor* Actor = GWorld->SpawnActor(ActorClass);
		UScriptComponent* ScriptComponent = static_cast<UScriptComponent*>(Actor->AddComponent(UScriptComponent::StaticClass()));
		ScriptComponent->SetScriptPath(ScriptName);
		ScriptComponent->BeginPlay();
		return Actor;
	};

	// ====================================================================
	// Player 
	// ====================================================================
	
	LuaState.new_enum("EKeyInput",
		"W", EKeyInput::W,
		"A", EKeyInput::A,
		"S", EKeyInput::S,
		"D", EKeyInput::D,
		"MouseLeft", EKeyInput::MouseLeft,
		"MouseRight", EKeyInput::MouseRight);

	LuaState["IsKeyDown"] = [](EKeyInput InputKey) -> bool
	{
		return UInputManager::GetInstance().IsKeyDown(InputKey);
	};
	LuaState["IsKeyPressed"] = [](EKeyInput InputKey) -> bool
	{
		return UInputManager::GetInstance().IsKeyPressed(InputKey);
	};
	LuaState["IsKeyReleased"] = [](EKeyInput InputKey) -> bool
	{
		return UInputManager::GetInstance().IsKeyReleased(InputKey);
	};
	LuaState["GetMouseDelta"] = []() -> FVector
		{
		return UInputManager::GetInstance().GetMouseDelta();
	};

	// 마우스 스크린 위치 가져오기
	LuaState["GetMousePosition"] = []() -> FVector2
	{
		FVector mousePos = UInputManager::GetInstance().GetMousePosition();
		return FVector2(mousePos.X, mousePos.Y);
	};

	// 스크린 좌표 -> 월드 위치 변환 (3D 레이캐스팅 기반)
	// 스크린 좌표에서 레이를 쏴서 Z=PlayerZ 평면과의 교차점을 계산
	LuaState["ScreenToWorldPosition"] = [](FVector2 ScreenPos, float PlayerZ) -> FVector2
	{
		auto& ViewportManager = UViewportManager::GetInstance();
		FViewportClient* ViewportClient = ViewportManager.GetClients()[ViewportManager.GetActiveIndex()];
		FViewport* Viewport = ViewportClient->GetOwningViewport();
		const FCameraConstants& CamConstant = ViewportClient->GetCamera()->GetFViewProjConstants();
		FRect Rect = Viewport->GetRect();

		// 1. Screen → Normalized Screen (0~1 범위)
		float NormalizedX = (ScreenPos.X - static_cast<float>(Rect.Left)) / static_cast<float>(Rect.Width);
		float NormalizedY = (ScreenPos.Y - static_cast<float>(Rect.Top)) / static_cast<float>(Rect.Height);

		// 2. Normalized Screen → NDC (-1~1 범위)
		// Y는 반전 (screen Y down = NDC Y up)
		float NDCX = NormalizedX * 2.0f - 1.0f;
		float NDCY = (1.0f - NormalizedY) * 2.0f - 1.0f;

		// 3. ViewProjection의 역행렬 계산 (한 번에)
		// Row-vector system: Clip = World * View * Proj
		// 역변환: World = Clip * (View * Proj)^-1
		FMatrix ViewProj = CamConstant.View * CamConstant.Projection;
		FMatrix InvViewProj = ViewProj.InverseGeneral();  // Use general 4x4 inverse for projection matrices

		// 4. Near plane (Z=0)과 Far plane (Z=1)에서 레이 포인트 생성
		FVector4 ClipNear = FVector4(NDCX, NDCY, 0.0f, 1.0f);
		FVector4 ClipFar = FVector4(NDCX, NDCY, 1.0f, 1.0f);

		// 5. Clip → World 변환
		FVector4 WorldNear = ClipNear * InvViewProj;
		FVector4 WorldFar = ClipFar * InvViewProj;

		// 6. Perspective divide
		FVector RayStart = FVector(WorldNear.X / WorldNear.W, WorldNear.Y / WorldNear.W, WorldNear.Z / WorldNear.W);
		FVector RayEnd = FVector(WorldFar.X / WorldFar.W, WorldFar.Y / WorldFar.W, WorldFar.Z / WorldFar.W);

		// 7. 레이 방향 계산
		FVector RayDir = RayEnd - RayStart;
		RayDir.Normalize();

		// 8. 레이와 Z = PlayerZ 평면의 교차점 계산
		// Ray: P = RayStart + t * RayDir
		// Plane: Z = PlayerZ
		// RayStart.Z + t * RayDir.Z = PlayerZ
		// t = (PlayerZ - RayStart.Z) / RayDir.Z
		float t = (PlayerZ - RayStart.Z) / RayDir.Z;
		FVector IntersectionPoint = RayStart + RayDir * t;

		return FVector2(IntersectionPoint.X, IntersectionPoint.Y);
	};

	// 스크린 좌표 -> 마우스 방향 변환 (탑다운 게임용 - 간단한 2D 매핑)
	// 화면 중앙을 기준으로 마우스 오프셋을 월드 XY 방향으로 변환
	// [DEPRECATED] ScreenToWorldPosition을 사용하세요
	LuaState["ScreenToWorldDirection"] = [](FVector2 ScreenPos) -> FVector2
	{
		auto& ViewportManager = UViewportManager::GetInstance();
		FViewportClient* ViewportClient = ViewportManager.GetClients()[ViewportManager.GetActiveIndex()];
		FViewport* Viewport = ViewportClient->GetOwningViewport();

		// 화면 크기 및 중앙 좌표 계산 (Rect.Left, Rect.Top 오프셋 포함)
		FRect Rect = Viewport->GetRect();
		float CenterX = static_cast<float>(Rect.Left) + static_cast<float>(Rect.Width) * 0.5f;
		float CenterY = static_cast<float>(Rect.Top) + static_cast<float>(Rect.Height) * 0.5f;

		// 화면 중앙으로부터의 오프셋
		float OffsetX = ScreenPos.X - CenterX;
		float OffsetY = ScreenPos.Y - CenterY;

		// 탑다운 뷰 매핑:
		// Screen X+ (오른쪽) → World Y+ (오른쪽)
		// Screen Y+ (아래) → World X- (뒤)
		// Screen Y- (위) → World X+ (앞)
		FVector2 WorldDir = FVector2(
			-OffsetY,  // Screen Y 반전 → World X
			OffsetX    // Screen X → World Y
		);

		// 정규화
		float Length = sqrtf(WorldDir.X * WorldDir.X + WorldDir.Y * WorldDir.Y);
		if (Length > 0.0f)
		{
			WorldDir.X /= Length;
			WorldDir.Y /= Length;
		}

		return WorldDir;
	};

	lua.new_usertype<UCamera>("Camera",
		"Location", sol::property(&UCamera::GetLocation, &UCamera::SetLocation),
		"Rotation", sol::property(&UCamera::GetRotation, &UCamera::SetRotation),
		"Forward", sol::property(&UCamera::GetForward),
		"Right", sol::property(&UCamera::GetRight),
		"Up", sol::property(&UCamera::GetUp)
	);

	// ====================================================================
	// ATopDownCameraActor - Top-down camera actor for PIE/Game mode
	// ====================================================================
	lua.new_usertype<ATopDownCameraActor>("TopDownCameraActor",
		sol::base_classes, sol::bases<AActor>(),
		"Location", sol::property(&ATopDownCameraActor::GetActorLocation, &ATopDownCameraActor::SetActorLocation),
		"Rotation", sol::property(&ATopDownCameraActor::GetRotation, &ATopDownCameraActor::SetRotation),
		"SetCameraOffset", &ATopDownCameraActor::SetCameraOffset,
		"SetFollowTarget", &ATopDownCameraActor::SetFollowTarget
	);
	LuaState["GetCamera"] = [](sol::this_state s)->sol::object
	{
		sol::state_view lua(s);

		// PIE/Game 모드인 경우 TopDownCameraActor 반환
		if (GWorld && (GWorld->GetWorldType() == EWorldType::PIE || GWorld->GetWorldType() == EWorldType::Game))
		{
			// World에서 APlayer 찾기
			APlayer* Player = GWorld->GetFirstPlayerActor();
			if (Player)
			{
				ATopDownCameraActor* CameraActor = Player->GetCameraActor();
				if (CameraActor)
				{
					return sol::make_object(lua, CameraActor);
				}
			}
		}

		// Editor 모드인 경우 UCamera 반환
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Clients = ViewportManager.GetClients();
		UCamera* EditorCamera = Clients[ViewportManager.GetActiveIndex()]->GetCamera();
		return sol::make_object(lua, EditorCamera);
	};
	LuaState["WorldToScreenPos"] = [](FVector WorldPos)->FVector2
	{
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Viewports = ViewportManager.GetViewports();
		const auto& Clients = ViewportManager.GetClients();
		FViewportClient* ViewportClient = Clients[ViewportManager.GetActiveIndex()];
		FViewport* Viewport = ViewportClient->GetOwningViewport();

		// PIE/Game 모드인 경우 PlayerCameraManager의 카메라 사용
		FCameraConstants CamConstant;
		if (GWorld && (GWorld->GetWorldType() == EWorldType::PIE || GWorld->GetWorldType() == EWorldType::Game))
		{
			APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
			if (CameraManager)
			{
				CamConstant = CameraManager->GetCameraConstants();
			}
			else
			{
				// Fallback to EditorCamera if PlayerCameraManager doesn't exist
				CamConstant = ViewportClient->GetCamera()->GetFViewProjConstants();
			}
		}
		else
		{
			// Editor 모드인 경우 EditorCamera 사용
			CamConstant = ViewportClient->GetCamera()->GetFViewProjConstants();
		}

		FVector4 ViewPos = FVector4(WorldPos, 1) * CamConstant.View;
		FVector4 ClipPos = ViewPos * CamConstant.Projection;
		FVector2 NDC = FVector2(ClipPos.X / ClipPos.W, ClipPos.Y / ClipPos.W);
		FVector2 ScreenUV = NDC * 0.5f + FVector2(0.5f, 0.5f);
		ScreenUV.Y = 1 - ScreenUV.Y;
		FRect Rect = Viewport->GetRect();
		FVector2 ScreenPos = FVector2(Rect.Left + Rect.Width * ScreenUV.X, Rect.Top + Rect.Height * ScreenUV.Y);
		return ScreenPos;
	};
	LuaState["GetViewportRect"] = []()->FVector4
	{
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Viewports = ViewportManager.GetViewports();
		const auto& Clients = ViewportManager.GetClients();
		FViewportClient* ViewportClient = Clients[ViewportManager.GetActiveIndex()];
		FViewport* Viewport = ViewportClient->GetOwningViewport();
		FRect Rect = Viewport->GetRect();
		return FVector4(static_cast<float>(Rect.Left), static_cast<float>(Rect.Top),
		                static_cast<float>(Rect.Width), static_cast<float>(Rect.Height));
	};
	LuaState["FindActorByName"] = [](const std::string& InName)->AActor*
		{
			FName Name = FName(InName);
			const TArray<AActor*>& Actors = GWorld->GetLevel()->GetLevelActors();
			for (AActor* Actor : Actors)
			{
				if (Actor->GetName() == Name)
				{
					return Actor;
				}
			}
			return nullptr;
		};


	UE_LOG_INFO("Lua core types registered (Vector, Quaternion, Actor, OverlapInfo, PrimitiveComponent)");
	
	// ====================================================================
	// Helper Functions
	// ====================================================================

	LuaState["StartCameraFade"] = [](float FromAlpha, float ToAlpha, float Duration, FVector Color)
	{
		APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
		if (!CameraManager)
		{
			UE_LOG_ERROR("StartCameraFade: 카메라 매니저를 찾을 수 없습니다.");
			return;
		}
		CameraManager->StartCameraFade(FromAlpha, ToAlpha, Duration, Color);
	};

	LuaState["SetVignette"] = [](FVector InVignetteColor, float InVignetteIntensity)
	{
		APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
		if (!CameraManager)
		{
			UE_LOG_ERROR("StartCameraFade: 카메라 매니저를 찾을 수 없습니다.");
			return;
		}
		FViewTarget& ViewTarget = CameraManager->GetViewTargetInfo();
		ViewTarget.CameraComponent->PostProcessSettings.bOverride_VignetteColor = true;
		ViewTarget.CameraComponent->PostProcessSettings.VignetteColor = InVignetteColor;
		ViewTarget.CameraComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
		ViewTarget.CameraComponent->PostProcessSettings.VignetteIntensity = InVignetteIntensity;
	};
	
	LuaState["ResetVignette"] = [](FVector InVignetteColor, float InVignetteIntensity)
	{
		APlayerCameraManager* CameraManager = GWorld->GetCameraManager();
		if (!CameraManager)
		{
			UE_LOG_ERROR("StartCameraFade: 카메라 매니저를 찾을 수 없습니다.");
			return;
		}
		FViewTarget& ViewTarget = CameraManager->GetViewTargetInfo();
		ViewTarget.CameraComponent->PostProcessSettings.bOverride_VignetteColor = false;
		ViewTarget.CameraComponent->PostProcessSettings.bOverride_VignetteIntensity = false;
	};

	LuaState["SetTimeDilation"] = [](float InTimeDilation)
	{
		UTimeManager::GetInstance().SetTimeDilation(InTimeDilation);
	};
	
	LuaState["Random"] = [](float MinValue, float MaxValue) -> float
	{
		std::random_device RandomDevice;
		std::mt19937 RandomGenerator(RandomDevice());
		std::uniform_real_distribution<float> Distribution(MinValue, MaxValue);
		return Distribution(RandomGenerator);
	};
}

void UScriptManager::RegisterGlobalFunctions()
{
	sol::state& lua = *LuaState;

	// Override Lua print to use engine console
	lua["print"] = [](sol::variadic_args args) {
		FString output;
		for (auto arg : args)
		{
			if (output.length() > 0)
				output += " ";

			sol::object obj = arg;
			sol::type argType = obj.get_type();

			// 타입별 처리
			if (argType == sol::type::number)
			{
				// 정수 또는 실수 처리
				if (obj.is<int32>())
					output += std::to_string(obj.as<int32>());
				else if (obj.is<double>())
					output += std::to_string(obj.as<double>());
				else
					output += std::to_string(obj.as<float>());
			}
			else if (argType == sol::type::string)
			{
				output += obj.as<std::string>();
			}
			else if (argType == sol::type::boolean)
			{
				output += (obj.as<bool>() ? "true" : "false");
			}
			else if (argType == sol::type::nil)
			{
				output += "nil";
			}
			else if (argType == sol::type::table)
			{
				output += "[table]";
			}
			else if (argType == sol::type::function)
			{
				output += "[function]";
			}
			else if (argType == sol::type::userdata)
			{
				// FVector 처리
				if (obj.is<FVector>())
				{
					FVector vec = obj.as<FVector>();
					char buffer[128];
					snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)", vec.X, vec.Y, vec.Z);
					output += buffer;
				}
				// FQuaternion 처리
				else if (obj.is<FQuaternion>())
				{
					FQuaternion quat = obj.as<FQuaternion>();
					char buffer[128];
					snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f, %.3f)", quat.X, quat.Y, quat.Z, quat.W);
					output += buffer;
				}
				else
				{
					output += "[userdata]";
				}
			}
			else
			{
				output += "[unknown]";
			}
		}
		UE_LOG("%s", output.c_str());
		};

	// Engine log function
	lua["ULog"] = [](const std::string& msg) {
		UE_LOG("%s", msg.c_str());
		};

	// Get delta time
	lua["GetDeltaTime"] = []() -> float {
		return UTimeManager::GetInstance().GetDeltaTime();
		};

	// Get total time
	lua["GetTime"] = []() -> float {
		return UTimeManager::GetInstance().GetGameTime();
		};

	lua["DrawText"] = [](const std::string& Text, const FVector2& ScreenPos, const FVector2& RectSize, const float Size, const FVector4& Color)
	{
		UGameUI::GetInstance().TextUI(Text, ScreenPos, RectSize, Size, Color);
	};
	lua["DrawGaugeBar"] = [](const FVector2& ScreenPos, const FVector2& Size, float GaugePercent, const FVector4& BGColor, const FVector4& GaugeColor)
	{
		UGameUI::GetInstance().GaugeBar(ScreenPos, Size, GaugePercent, BGColor, GaugeColor);
	};


	UE_LOG_INFO("Lua global functions registered");
}

bool UScriptManager::CopyScriptFromEngineToBuild(const FString& ScriptPath)
{
	UPathManager& PathMgr = UPathManager::GetInstance();

	// 소스 경로: Engine/Data/Scripts
	path SourcePath = PathMgr.GetEngineDataPath() / "Scripts" / ScriptPath.c_str();

	// 대상 경로: Build/[Config]/Data/Scripts
	path DestPath = PathMgr.GetDataPath() / "Scripts" / ScriptPath.c_str();

	// 소스 파일이 존재하는지 확인
	if (!std::filesystem::exists(SourcePath))
	{
		UE_LOG_WARNING("CopyScript: 소스 스크립트를 찾을 수 없음: %s", SourcePath.string().c_str());
		return false;
	}
	try
	{
		// 대상 디렉토리 생성 (없는 경우)
		path DestDir = DestPath.parent_path();
		if (!std::filesystem::exists(DestDir))
		{
			std::filesystem::create_directories(DestDir);
			UE_LOG_INFO("CopyScript: 대상 디렉토리 생성: %s", DestDir.string().c_str());
		}

		// 파일 복사 (덮어쓰기)
		std::filesystem::copy_file(SourcePath, DestPath, std::filesystem::copy_options::overwrite_existing);

		UE_LOG_SUCCESS("CopyScript: 스크립트 복사 완료 - %s → Build/Data/Scripts/%s",
			ScriptPath.c_str(), ScriptPath.c_str());

		return true;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("CopyScript: 스크립트 복사 중 오류 발생: %s", e.what());
		return false;
	}
}

/*-----------------------------------------------------------------------------
	Hot Reload System
-----------------------------------------------------------------------------*/

TSet<FString> UScriptManager::GatherHotReloadTargets()
{
	TSet<FString> HotReloadTargets;

	UPathManager& PathMgr = UPathManager::GetInstance();

	for (const auto& Pair : LuaScriptMap)
	{
		const FString& ScriptPath = Pair.first;
		const auto& CachedLastWriteTime = Pair.second.LastCompileTime;

		// ✅ 새 정책: Engine 스크립트 변경 감지 → Build로 자동 복사
		path EngineScriptPath = PathMgr.GetEngineDataPath() / "Scripts" / ScriptPath.c_str();
		path BuildScriptPath = PathMgr.GetDataPath() / "Scripts" / ScriptPath.c_str();

		bool bNeedsCopy = false;

		// Engine 경로에 스크립트가 존재하는 경우
		if (std::filesystem::exists(EngineScriptPath))
		{
			std::error_code ErrorCode;
			auto EngineLastWriteTime = std::filesystem::last_write_time(EngineScriptPath, ErrorCode);

			if (!ErrorCode)
			{
				// Engine 스크립트가 변경되었는지 확인
				if (EngineLastWriteTime != CachedLastWriteTime)
				{
					// Engine에서 Build로 복사
					if (CopyScriptFromEngineToBuild(ScriptPath))
					{
						bNeedsCopy = true;
						UE_LOG_INFO("GatherHotReloadTargets: Engine 스크립트 변경 감지 및 복사 완료 - %s", ScriptPath.c_str());
					}
				}
			}
		}

		// Build 경로 확인 (복사 후 또는 독립적으로 존재)
		if (!std::filesystem::exists(BuildScriptPath))
		{
			continue;
		}

		// 현재 Build 스크립트 수정 시간 조회
		std::error_code ErrorCode;
		auto BuildLastWriteTime = std::filesystem::last_write_time(BuildScriptPath, ErrorCode);
		if (ErrorCode)
		{
			continue;
		}

		// Build 스크립트가 변경되었으면 Hot Reload 대상 추가
		if (BuildLastWriteTime != CachedLastWriteTime || bNeedsCopy)
		{
			HotReloadTargets.insert(ScriptPath);
		}
	}

	return HotReloadTargets;
}

void UScriptManager::RegisterScriptComponent(const FString& ScriptPath, UScriptComponent* Component)
{
	if (!Component)
	{
		return;
	}

	auto& Components = ScriptComponentRegistry[ScriptPath];

	// 중복 등록 방지
	if (std::find(Components.begin(), Components.end(), Component) == Components.end())
	{
		Components.push_back(Component);
	}
}

void UScriptManager::UnregisterScriptComponent(const FString& ScriptPath, UScriptComponent* Component)
{
	if (!Component)
	{
		return;
	}

	auto It = ScriptComponentRegistry.find(ScriptPath);
	if (It != ScriptComponentRegistry.end())
	{
		auto& Components = It->second;
		Components.erase(std::remove(Components.begin(), Components.end(), Component), Components.end());

		// 컴포넌트가 없으면 엔트리 제거
		if (Components.empty())
		{
			ScriptComponentRegistry.erase(It);
		}
	}
}

void UScriptManager::HotReloadScripts()
{
	// Hot Reload 체크 주기 제한 (성능 최적화)
	float CurrentTime = UTimeManager::GetInstance().GetGameTime();
	float TimeSinceLastCheck = CurrentTime - LastHotReloadCheckTime;

	if (TimeSinceLastCheck < HotReloadCheckInterval)
	{
		// 아직 체크 주기가 안 됨
		return;
	}

	// 체크 시간 업데이트
	LastHotReloadCheckTime = CurrentTime;

	TSet<FString> HotReloadTargets = GatherHotReloadTargets();

	if (HotReloadTargets.empty())
	{
		return;
	}

	UE_LOG_INFO("ScriptManager: Hot Reloading %d script(s)...", static_cast<int32>(HotReloadTargets.size()));

	for (const FString& ScriptPath : HotReloadTargets)
	{
		auto It = LuaScriptMap.find(ScriptPath);
		if (It == LuaScriptMap.end())
		{
			continue;
		}

		// 스크립트 리로드
		LoadLuaScript(ScriptPath);

		// 이 스크립트를 사용하는 모든 컴포넌트에게 알림
		auto ComponentIt = ScriptComponentRegistry.find(ScriptPath);
		if (ComponentIt != ScriptComponentRegistry.end())
		{
			const TArray<UScriptComponent*>& Components = ComponentIt->second;
			sol::table NewGlobalTable = It->second.GlobalTable;

			UE_LOG_INFO("  Notifying %d component(s) using script '%s'",
				static_cast<int32>(Components.size()), ScriptPath.c_str());

			for (UScriptComponent* Component : Components)
			{
				if (Component)
				{
					Component->OnScriptReloaded(NewGlobalTable);
				}
			}
		}
	}
}
