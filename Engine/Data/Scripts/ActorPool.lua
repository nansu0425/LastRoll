-- ==============================================================================
-- ActorPool.lua
-- ==============================================================================
-- 이 파일은 'Actor' 객체를 재사용하기 위한 오브젝트 풀(Object Pool)을 구현한
-- 싱글톤 모듈이다.
--
-- 액터의 스크립트 리소스 경로(ActorName)를 키(Key)로 사용하여
-- 스크립트 유형별로 풀을 나누어 관리한다.
--
-- [주요 함수]
-- 1. Get(ActorName):
--    - 비활성 풀('InactiveActors')에 재사용 가능한 액터가 있는지 확인한다.
--    - 있으면 해당 액터를 활성화(Tick, 렌더링, 충돌 활성화)하여 반환한다.
--    - 없으면 'SpawnActorFromScript'를 호출하여 새 액터를 생성하고,
--      'ActorToNameMap'에 등록한 뒤 반환한다.
--
-- 2. Return(Actor):
--    - 사용이 끝난 액터를 비활성화(Tick, 렌더링, 충돌 비활성화)한다.
--    - 'ActorToNameMap'을 참조하여 이 액터를 올바른 스크립트 유형의
--      비활성 풀에 다시 저장(table.insert)한다.
-- ==============================================================================

local ActorPool = {}
ActorPool.__index = ActorPool

-- 정적 생성자 (싱글톤)
local Instance = nil

---
-- 내부 자료 구조
-- self.InactiveActors - 스크립트 경로별 비활성 객체 목록
-- self.ActorToNameMap - 액터와 이름 매핑
---
function ActorPool:Create()
    local Pool = {}
    setmetatable(Pool, ActorPool)
    Pool.InactiveActors = {}
    Pool.ActorToNameMap = {}
    return Pool
end

---
-- @brief 지정된 유형의 액터를 풀에서 가져온다.
-- @param ActorName (string) - 액터의 이름
-- @return Actor - 활성화된 액터
---
function ActorPool:Get(ActorName)
    if not ActorName then 
        print("ActorPool:Get - ActorName이 Nil입니다.")
        return nil
    end

    if not self.InactiveActors[ActorName] then 
        self.InactiveActors[ActorName] = {}
    end

    local TargetInactiveActors = self.InactiveActors[ActorName]
    
    -- 풀에서 비활성 액터를 가져옴 (pop) 
    local ActorToGet = table.remove(TargetInactiveActors)
    
    if ActorToGet then 
        --print("Get")
        ActorToGet:SetCanTick(true)
        ActorToGet:SetActorHiddenInGame(false)
        ActorToGet:SetActorEnableCollision(true)
    else
        ActorToGet = SpawnActorByName(ActorName)
        
        if ActorToGet then 
            --print("Spawn")
            ActorToGet:SetCanTick(true)
            ActorToGet:SetActorHiddenInGame(false)
            ActorToGet:SetActorEnableCollision(true)
            
            self.ActorToNameMap[ActorToGet.UUID] = ActorName
        else
            --print("ActorPool:Get - SpawnActorFromScript 실패: " .. ActorName)
            return nil
        end
    end

    return ActorToGet
end

---
-- @brief 사용이 끝난 액터를 반환한다.
-- @param ActorToReturn (Actor) - 반납할 액터 
---
function ActorPool:Return(ActorToReturn)
    if not ActorToReturn then 
        --print ("ActorPool:Return - 반납할 Actor가 nil입니다.")
        return
    end
    
    local ActorName = self.ActorToNameMap[ActorToReturn.UUID]
    if not ActorName then 
        --print("ActorPool:Return - 이 액터는 풀에서 관리하는 객체가 아닙니다.")
        return
    end

    if not self.InactiveActors[ActorName] then 
        self.InactiveActors[ActorName] = {}
    end

    --- 액터 비활성화
    ActorToReturn:SetCanTick(false)
    ActorToReturn:SetActorHiddenInGame(true) 
    ActorToReturn:SetActorEnableCollision(false)
    ActorToReturn:StopAllCoroutine()
    
    table.insert(self.InactiveActors[ActorName], ActorToReturn)
end

---
-- @brief 풀의 모든 내용을 비운다.
-- @note PIE 혹은 새로운 세션 시작 시 명시적으로 호출해야한다.
---
function ActorPool:Clear()
    print("Clear")
    self.InactiveActors = {}
    self.ActorToNameMap = {}
end

-- 싱글톤 인스턴스 생성
if not Instance then 
    Instance = ActorPool.Create()
end
    
return Instance