#include "pch.h"
#include "Manager/Script/Public/ScriptManager.h"


// 엔진 인클루드
#include <random>

#include "Global/Vector.h"
#include "Global/Quaternion.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Public/SceneComponent.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/PrimitiveComponent.h"
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

		try
		{
			// lua globals를 fallback으로 하는 environment 생성
			// 스크립트의 함수 정의들은 env에 저장되고, 찾지 못한 것은 globals에서 찾음
			sol::environment env(*LuaState, sol::create, LuaState->globals());

			// environment 내에서 스크립트 실행
			LuaState->script_file(BuildScriptPath.string(), env);

			// env 자체가 sol::table을 상속하므로 바로 저장 가능
			// env에는 스크립트에서 정의한 모든 함수가 들어있음 (Tick, BeginPlay 등)
			LuaScriptMap[ScriptPath].Path = ScriptPath;
			LuaScriptMap[ScriptPath].LastCompileTime = LastWriteTime;
			LuaScriptMap[ScriptPath].GlobalTable = env;
		}
		catch (const sol::error& e)
		{
			UE_LOG_ERROR("Lua compile/load error: %s", e.what());
			return false;
		}

		UE_LOG_INFO("ScriptManager: Hot Reload 등록 완료 - %s", ScriptPath.c_str());
		return true;
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
		)
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
		"PrintLocation", [](AActor* self) {
			FVector loc = self->GetActorLocation();
			UE_LOG("Actor %s Location: (%.2f, %.2f, %.2f)",
				self->GetName().ToString().c_str(), loc.X, loc.Y, loc.Z);
		},
		"SetCanTick", &AActor::SetCanTick,
		"SetActorHiddenInGame", &AActor::SetActorHiddenInGame,
		"SetActorEnableCollision", &AActor::SetActorEnableCollision,
		"StopAllCoroutine", &AActor::StopAllCoroutine
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
				[](const FOverlapInfo& Info) { UE_LOG("Hello World!"); }
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
		"D", EKeyInput::D);

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
	lua.new_usertype<UCamera>("Camera",
		"Location", sol::property(&UCamera::GetLocation, &UCamera::SetLocation),
		"Rotation", sol::property(&UCamera::GetRotation, &UCamera::SetRotation),
		"Forward", sol::property(&UCamera::GetForward),
		"Right", sol::property(&UCamera::GetRight),
		"Up", sol::property(&UCamera::GetUp)
	);
	LuaState["GetCamera"] = []()->UCamera*
	{
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Viewports = ViewportManager.GetViewports();
		const auto& Clients = ViewportManager.GetClients();
		return Clients[ViewportManager.GetActiveIndex()]->GetCamera();
	};
	LuaState["WorldToScreenPos"] = [](FVector WorldPos)->FVector2
	{
		auto& ViewportManager = UViewportManager::GetInstance();
		const auto& Viewports = ViewportManager.GetViewports();
		const auto& Clients = ViewportManager.GetClients();
		FViewportClient* ViewportClient = Clients[ViewportManager.GetActiveIndex()];
		FViewport* Viewport = ViewportClient->GetOwningViewport();
		const FCameraConstants& CamConstant = ViewportClient->GetCamera()->GetFViewProjConstants();

		FVector4 ViewPos = FVector4(WorldPos, 1) * CamConstant.View;
		FVector4 ClipPos = ViewPos * CamConstant.Projection;
		FVector2 NDC = FVector2(ClipPos.X / ClipPos.W, ClipPos.Y / ClipPos.W);
		FVector2 ScreenUV = NDC * 0.5f + FVector2(0.5f, 0.5f);
		ScreenUV.Y = 1 - ScreenUV.Y;
		FRect Rect = Viewport->GetRect();
		FVector2 ScreenPos = FVector2(Rect.Left + Rect.Width * ScreenUV.X, Rect.Top + Rect.Height * ScreenUV.Y);
		return ScreenPos;
	};



	UE_LOG_INFO("Lua core types registered (Vector, Quaternion, Actor, OverlapInfo, PrimitiveComponent)");

	// ====================================================================
	// Helper Functions
	// ====================================================================
	
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

	lua["DrawText"] = [](const std::string& Text, const FVector2& ScreenPos, const float Size, const FVector4& Color)
	{
		UGameUI::GetInstance().TextUI(Text, ScreenPos, Size, Color);
	};
	lua["DrawHPBar"] = [](const FVector2& ScreenPos, const FVector2& Size, float HPPer)
	{
		UGameUI::GetInstance().HPBar(ScreenPos, Size, HPPer);
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
