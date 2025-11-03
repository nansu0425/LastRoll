#include "pch.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Script/Public/ScriptManager.h"
#include "Manager/Path/Public/PathManager.h"
#include "Actor/Public/Actor.h"
#include "json.hpp"

IMPLEMENT_CLASS(UScriptComponent, UActorComponent)

UScriptComponent::UScriptComponent()
	: ScriptPath("")
{
	// Tick 활성화
	bCanEverTick = true;
}

UScriptComponent::~UScriptComponent()
{
	StopAllCoroutine();

	// CRITICAL: ScriptManager에서 등록 해제 (댕글링 포인터 방지)
	// EndPlay()가 호출되지 않는 경우를 대비 (Editor에서 컴포넌트 삭제 시)
	if (!ScriptPath.empty())
	{
		UScriptManager::GetInstance().UnregisterScriptComponent(ScriptPath, this);
	}

	// CleanupLuaResources();
}

void UScriptComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ScriptPath.empty())
	{
		return;
	}

	SetInstanceTable(UScriptManager::GetInstance().GetTable(ScriptPath));

	// ScriptManager에 Hot Reload 알림을 받기 위해 등록
	UScriptManager::GetInstance().RegisterScriptComponent(ScriptPath, this);

	// BeginPlay 호출
	CallLuaFunction("BeginPlay");
}

void UScriptComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	CallLuaFunction("Tick", DeltaTime);

	//등록 대기중인 코루틴 등록
	for (auto& pendingData : PendingCoroutines)
	{
		MakeCoroutine(pendingData.FuncName.ToString());
	}
	PendingCoroutines.clear();

	int32 CoroutineCount = static_cast<int32>(Coroutines.size());

	for (int32 i = static_cast<int32>(Coroutines.size()) - 1; i >= 0; --i)
	{
		CoroutineData& Data = Coroutines[i];
		bool bEnd = false;
		if (Data.WaitCondition.WaitType == EWaitType::Time)
		{
			Data.WaitCondition.WaitTime -= DeltaTime;
			if (Data.WaitCondition.WaitTime <= 0)
			{
				Data.WaitCondition.WaitType = EWaitType::None;
				bEnd = ResumeCoroutine(Data);
			}
		}
		else if (Data.WaitCondition.WaitType == EWaitType::Lambda) {
			if (Data.WaitCondition.Lambda && Data.WaitCondition.Lambda()) {
				Data.WaitCondition.WaitType = EWaitType::None;
				bEnd = ResumeCoroutine(Data);
			}
		}
		else if (Data.WaitCondition.WaitType == EWaitType::None)
		{
			bEnd = ResumeCoroutine(Data);
		}


		if (bEnd)
		{
			//swap remove 방식으로 변경필요
			Coroutines.erase(Coroutines.begin() + i);
		}
	}


}

void UScriptComponent::EndPlay()
{
	// EndPlay 호출
	CallLuaFunction("EndPlay");

	// ScriptManager에서 등록 해제
	if (!ScriptPath.empty())
	{
		UScriptManager::GetInstance().UnregisterScriptComponent(ScriptPath, this);
	}

	Super::EndPlay();
}

void UScriptComponent::TriggerOnOverlap(AActor* OtherActor)
{
	CallLuaFunction("OnOverlap", OtherActor);
}

void UScriptComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("ScriptPath"))
		{
			ScriptPath = InOutHandle["ScriptPath"].ToString();
			// NOTE: SetInstanceTable은 BeginPlay에서 호출됨
			// Serialize에서 호출하면 RegisterScriptComponent가 누락되어 Hot reload 알림을 받지 못함
		}
	}
	else
	{
		InOutHandle["ScriptPath"] = ScriptPath;
	}
}

UObject* UScriptComponent::Duplicate()
{
	UScriptComponent* DuplicatedComp = Cast<UScriptComponent>(Super::Duplicate());

	// ScriptPath 복사 (Super::Duplicate()는 NewObject()를 호출하여 생성자에서 초기화되므로 명시적 복사 필요)
	DuplicatedComp->ScriptPath = ScriptPath;

	// PIE 복제 시 Lua 리소스는 초기화하지 않음
	// BeginPlay()에서 각 World별로 독립적인 리소스 생성
	// (InstanceEnv와 CachedFunctions는 BeginPlay에서 SetInstanceTable을 통해 재생성)

	return DuplicatedComp;
}

void UScriptComponent::SetInstanceTable(const sol::table GlobalTable)
{
	UScriptManager& ScriptMgr = UScriptManager::GetInstance();
	sol::state& lua = ScriptMgr.GetLuaState();

	// 1. Instance Environment 생성 (GlobalTable을 fallback으로)
	//    Environment chain: InstanceEnv -> GlobalTable(ScriptEnv) -> lua.globals()
	InstanceEnv = sol::environment(lua, sol::create, GlobalTable);

	InstanceEnv["self"] = this;

	// 2. Instance 데이터 설정
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// obj를 Proxy Table로 생성 (Actor 접근 + 동적 프로퍼티 저장 지원)
		sol::table objProxy = lua.create_table();

		// _actor 필드에 실제 Actor 포인터 저장 (내부용)
		objProxy["_actor"] = Owner;

		// Metatable 설정
		sol::table mt = lua.create_table();

		// __index: Actor property 또는 동적 프로퍼티 읽기
		// CRITICAL: owner를 캡처하지 않고 매번 테이블에서 가져옴 (댕글링 포인터 방지)
		mt[sol::meta_function::index] = [](sol::table self, const std::string& key) -> sol::object {
			sol::state_view lua = self.lua_state();

			// 동적 프로퍼티 먼저 확인 (raw_get으로 메타메서드 우회)
			sol::object val = self.raw_get<sol::object>(key);
			if (val.valid() && val.get_type() != sol::type::lua_nil)
			{
				return val;
			}

			// Actor 포인터를 매번 테이블에서 가져옴 (댕글링 포인터 방지)
			AActor* owner = self.raw_get<AActor*>("_actor");
			if (!owner)
				return sol::lua_nil;

			// Actor의 기본 프로퍼티 처리
			if (key == "Location")
			{
				return sol::make_object(lua, owner->GetActorLocation());
			}
			else if (key == "Rotation")
			{
				return sol::make_object(lua, owner->GetActorRotation());
			}
			else if (key == "UUID")
			{
				return sol::make_object(lua, owner->GetUUID());
			}
			else if (key == "Name")
			{
				return sol::make_object(lua, owner->GetName().ToString());
			}

			return sol::lua_nil;
		};

		// __newindex: Actor property 또는 동적 프로퍼티 쓰기
		// CRITICAL: owner를 캡처하지 않고 매번 테이블에서 가져옴 (댕글링 포인터 방지)
		mt[sol::meta_function::new_index] = [](sol::table self, const std::string& key, sol::object value) {
			// Actor 포인터를 매번 테이블에서 가져옴 (댕글링 포인터 방지)
			AActor* owner = self.raw_get<AActor*>("_actor");
			if (!owner)
				return;

			// Actor의 기본 프로퍼티 처리
			if (key == "Location")
			{
				FVector newLoc = value.as<FVector>();
				owner->SetActorLocation(newLoc);
			}
			else if (key == "Rotation")
			{
				FQuaternion newRot = value.as<FQuaternion>();
				owner->SetActorRotation(newRot);
			}
			else
			{
				// 동적 프로퍼티를 테이블에 저장 (Velocity, Speed, OverlapCount 등)
				self.raw_set(key, value);
			}
		};

		objProxy[sol::metatable_key] = mt;

		// InstanceEnv에 obj 등록
		InstanceEnv["obj"] = objProxy;

		// InstanceEnv에 Owner 등록
		InstanceEnv["Owner"] = Owner;

		// UUID는 직접 접근 가능하도록 (선택사항)
		InstanceEnv["UUID"] = Owner->GetUUID();

		// Helper 함수 - this를 캡처하여 매번 GetOwner()로 가져옴 (댕글링 포인터 방지)
		InstanceEnv["GetLocation"] = [this]()
		{
			AActor* Owner = GetOwner();
			return Owner ? Owner->GetActorLocation() : FVector(0, 0, 0);
		};
		InstanceEnv["SetLocation"] = [this](const FVector& v)
		{
			AActor* Owner = GetOwner();
			if (Owner) Owner->SetActorLocation(v);
		};
		InstanceEnv["PrintLocation"] = [this]()
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				FVector loc = Owner->GetActorLocation();
				UE_LOG("Location: (%.2f, %.2f, %.2f)", loc.X, loc.Y, loc.Z);
			}
		};
		InstanceEnv["SetCanTick"] = [this](bool bInCanEverTick)
		{
			AActor* Owner = GetOwner();
			Owner->SetCanTick(bInCanEverTick);
			UE_LOG("SetCanTick!");
		};
		InstanceEnv["SetActorHiddenInGame"] = [this](bool bInHidden)
		{
			AActor* Owner = GetOwner();
			Owner->SetActorHiddenInGame(bInHidden);
			UE_LOG("SetActorHiddenGame!");
		};
		InstanceEnv["SetActorEnableCollision"] = [this](bool bInEnableCollision)
		{
			AActor* Owner = GetOwner();
			Owner->SetActorEnableCollision(bInEnableCollision);
			UE_LOG("SetActorEnableCollision!");
		};
		InstanceEnv["GetComponent"] = [this](const FString& ClassName) -> UActorComponent*
		{
			AActor* Owner = GetOwner();
			UClass* TargetClass = UClass::FindClass(ClassName);
			if (!TargetClass)
			{
				UE_LOG_WARNING("Actor:GetComponent: UClass '%s'를 찾을 수 없습니다.", ClassName.c_str());
				return nullptr;
			}

			return Owner->GetComponentByClass(TargetClass);
		};
		InstanceEnv["GetPrimitiveComponentByName"] = [this](const FString& ComponentName) -> UPrimitiveComponent*
		{
			AActor* Owner = GetOwner();
			for (UActorComponent* ActorComponent : Owner->GetOwnedComponents())
			{
				if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent))
				{
					if (PrimitiveComponent->GetName().ToString() == ComponentName)
					{
						return PrimitiveComponent;
					}
				}
			}
			return nullptr;
		};

		//StartCoroutine
		InstanceEnv["StartCoroutine"] = [this](const FString& FuncName)
			{
				RegisterPendingCoroutine(FuncName);
			};

		//StopCoroutine
		InstanceEnv["StopCoroutine"] = [this](const FString& FuncName)
			{
				StopCoroutine(FuncName);
			};
	}

	// 3. 스크립트 함수들을 캐싱하고 environment 설정
	//    이렇게 하면 함수 호출 시마다 environment를 설정할 필요 없음
	CachedFunctions.clear();

	const char* FunctionNames[] = { "BeginPlay", "Tick", "EndPlay", "OnBeginOverlap", "OnEndOverlap" };
	for (const char* FuncName : FunctionNames)
	{
		sol::optional<sol::function> func_opt = GlobalTable[FuncName];
		if (func_opt)
		{
			sol::function func = *func_opt;
			// 함수의 environment를 InstanceEnv로 설정
			// 이제 함수 내에서 obj, UUID 등에 직접 접근 가능
			InstanceEnv.set_on(func);
			CachedFunctions[FString(FuncName)] = func;
		}
	}

	UE_LOG_DEBUG("ScriptComponent: Instance environment 초기화 완료 (%d functions cached)",
	             static_cast<int32>(CachedFunctions.size()));
}

void UScriptComponent::OnScriptReloaded(const sol::table& NewGlobalTable)
{
	UE_LOG_DEBUG("ScriptComponent: Hot reload notification received for '%s'", ScriptPath.c_str());

	// 새 GlobalTable로 InstanceEnv와 CachedFunctions 재생성
	SetInstanceTable(NewGlobalTable);

	// Hot reload 후 BeginPlay()를 다시 호출하여 동적 프로퍼티 재초기화
	// (obj.Velocity, obj.Speed 등 스크립트에서 설정한 변수들)
	//CallLuaFunction("BeginPlay");

	UE_LOG_SUCCESS("ScriptComponent: Hot reload complete for '%s'", ScriptPath.c_str());
}
/*-----------------------------------------------------------------------------
	Overlap Delegate Binding
-----------------------------------------------------------------------------*/

void UScriptComponent::BindOverlapDelegates()
{
	AActor* Owner = GetOwner();
	/*if (!Owner || !bScriptLoaded)
		return;*/
	if (!Owner)
		return;

	// Owner Actor의 모든 Component 순회
	const TArray<UActorComponent*>& Components = Owner->GetOwnedComponents();

	for (UActorComponent* Component : Components)
	{
		// PrimitiveComponent만 처리
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component);
		if (!PrimComp)
			continue;

		// *** CRITICAL: Overlap 이벤트 생성 활성화 ***
		// bGenerateOverlapEvents가 false면 UpdateOverlaps()가 바로 리턴하므로
		// 델리게이트 바인딩만으로는 충분하지 않음 - 반드시 활성화 필요
		PrimComp->SetGenerateOverlapEvents(true);

		// BeginOverlap 델리게이트 바인딩
		FDelegateHandle BeginHandle = PrimComp->OnComponentBeginOverlap.AddWeakLambda(
			this,  // UObject for weak reference
			[this](const FOverlapInfo& OverlapInfo)
			{
				OnBeginOverlapCallback(OverlapInfo);
			}
		);

		// 핸들 저장 (나중에 해제용)
		BeginOverlapHandles.push_back(TPair<UPrimitiveComponent*, FDelegateHandle>(PrimComp, BeginHandle));

		// EndOverlap 델리게이트 바인딩
		FDelegateHandle EndHandle = PrimComp->OnComponentEndOverlap.AddWeakLambda(
			this,  // UObject for weak reference
			[this](const FOverlapInfo& OverlapInfo)
			{
				OnEndOverlapCallback(OverlapInfo);
			}
		);

		// 핸들 저장
		EndOverlapHandles.push_back(TPair<UPrimitiveComponent*, FDelegateHandle>(PrimComp, EndHandle));
	}

	UE_LOG_DEBUG("ScriptComponent: Overlap 델리게이트 바인딩 완료 (%d PrimitiveComponents, GenerateOverlapEvents=true)",
	             static_cast<int32>(BeginOverlapHandles.size()));
}

void UScriptComponent::UnbindOverlapDelegates()
{
	// BeginOverlap 핸들 제거
	for (const auto& Pair : BeginOverlapHandles)
	{
		UPrimitiveComponent* Comp = Pair.first;
		FDelegateHandle Handle = Pair.second;

		if (Comp && Handle.IsValid())
		{
			Comp->OnComponentBeginOverlap.Remove(Handle);
		}
	}
	BeginOverlapHandles.clear();

	// EndOverlap 핸들 제거
	for (const auto& Pair : EndOverlapHandles)
	{
		UPrimitiveComponent* Comp = Pair.first;
		FDelegateHandle Handle = Pair.second;

		if (Comp && Handle.IsValid())
		{
			Comp->OnComponentEndOverlap.Remove(Handle);
		}
	}
	EndOverlapHandles.clear();
}

void UScriptComponent::OnBeginOverlapCallback(const FOverlapInfo& OverlapInfo)
{
	/*if (!bScriptLoaded)
		return;*/

	// 상대 Actor 가져오기
	AActor* OtherActor = OverlapInfo.OverlappingComponent ?
	                     OverlapInfo.OverlappingComponent->GetOwner() : nullptr;

	if (!OtherActor)
		return;

	// Lua 함수 호출: OnBeginOverlap(OtherActor)
	CallLuaFunction("OnBeginOverlap", OtherActor);
}

void UScriptComponent::OnEndOverlapCallback(const FOverlapInfo& OverlapInfo)
{
	/*if (!bScriptLoaded)
		return;*/

	// 상대 Actor 가져오기
	AActor* OtherActor = OverlapInfo.OverlappingComponent ?
	                     OverlapInfo.OverlappingComponent->GetOwner() : nullptr;

	if (!OtherActor)
		return;

	// Lua 함수 호출: OnEndOverlap(OtherActor)
	CallLuaFunction("OnEndOverlap", OtherActor);
}

void UScriptComponent::RegisterPendingCoroutine(const FString& FuncName)
{
	PendingCoroutines.push_back(PendingCoroutineData{ FuncName});
}

void UScriptComponent::MakeCoroutine(const FString& FuncName)
{
	sol::state& LuaState = UScriptManager::GetInstance().GetLuaState();

	CoroutineData Data;
	Data.Thread = sol::thread::create(LuaState);
	sol::function Func = InstanceEnv[FuncName];
	if (!Func.valid()) {
		UE_LOG("Function not found %s", FuncName.c_str());
		return;
	}
	Data.Coroutine = sol::coroutine(Data.Thread.state(), Func);
	Data.GCRef = sol::make_reference(LuaState, Data.Thread);
	Data.FuncName = FName(FuncName);

	Coroutines.push_back(Data);
}

void UScriptComponent::StopCoroutine(const FString& FuncName)
{
	FName Name = FName(FuncName);
	for (int32 i = static_cast<int32>(Coroutines.size()) - 1; i >= 0; --i)
	{
		if (Coroutines[i].FuncName == Name)
		{
			Coroutines.erase(Coroutines.begin() + i);
		}
	}
}



//끝났는지 여부 리턴 true = 끝남
bool UScriptComponent::ResumeCoroutine(CoroutineData& InData)
{
	FString name = InData.FuncName.ToString();
	auto YieldResult = InData.Coroutine();

	sol::thread_status status = InData.Thread.status();

	sol::call_status Status = InData.Coroutine.status();
	if (Status == sol::call_status::yielded)
	{
		if (YieldResult.return_count() > 0)
		{
			InData.WaitCondition = YieldResult[0];
			//UE_LOG("Coroutine Resume : %s", InData.FuncName.ToString().c_str());
		}
	}
	else if (Status == sol::call_status::ok)
	{
		//UE_LOG("Coroutine End : %s", InData.FuncName.ToString().c_str());

		return true;
	}
	else
	{
		sol::error err = YieldResult;
		UE_LOG("Coroutine Error : %s \n Reason : %s", InData.FuncName.ToString().c_str(), err.what());
		return true;
	}
	return false;
}


void UScriptComponent::StopAllCoroutine()
{
	Coroutines.clear();
	PendingCoroutines.clear();
}