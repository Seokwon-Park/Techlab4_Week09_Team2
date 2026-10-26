-- IceEscape 전용 Stage Goal. stage_goal.lua와 같은 일을 하지만 다음 Stage 위치를 계산하지 않는다.
-- 골 액터 이름(StageN_Goal)에서 번호를 읽고 StageN+1_Start 액터 위치로 보낸다.
-- 다음 Start가 없으면 마지막 Stage다. 그래서 Stage 개수나 간격이 바뀌어도 이 스크립트는 그대로 쓴다.
local SPAWN_HEIGHT = 0.5
local state = script_state

function BeginPlay()
    state.used = false
end

function OnBeginOverlap(other)
    if state.used or other.Name ~= "IcePlayer" then
        return
    end

    local stageNumber = tonumber(string.match(obj.Name, "^Stage(%d+)_Goal$"))
    if not stageNumber then
        print("[Stage] 골 액터 이름에서 Stage 번호를 읽지 못함: " .. obj.Name)
        return
    end

    state.used = true
    local nextStart = GetActorByName("Stage" .. (stageNumber + 1) .. "_Start")
    if not nextStart then
        print("[Stage] 모든 Stage 완료")
        return
    end

    local nextSpawn = nextStart.Location + FVector(0, 0, SPAWN_HEIGHT)
    _G.RespawnLocation = nextSpawn

    other:ResetMovement()
    other.Velocity = FVector(0, 0, 0)
    other.Location = nextSpawn
    _G.bIsMoving = false

    local events = _G.GameEvents
    if events and events.ResetCamera then
        events.ResetCamera()
    end
    print("[Stage] " .. stageNumber .. " -> " .. (stageNumber + 1))
end
