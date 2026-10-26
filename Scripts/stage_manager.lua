-- 한 명의 Player가 순서대로 진행하는 최소 Stage 관리자.
local state = script_state

-- 새 Stage가 생기면 시작 위치만 추가한다.
local stageStarts = {
    [1] = FVector(0, 0, 0.5),
    [2] = FVector(0, 1000, 0.5),
    [3] = FVector(0, 2000, 0.5),
    [4] = FVector(0, 3000, 0.5),
    [5] = FVector(0, 4000, 0.5),

    
}

local function Advance(player)
    if player.Name ~= "IcePlayer" then
        return false
    end

    local nextStage = state.currentStage + 1
    local nextLocation = stageStarts[nextStage]
    if not nextLocation then
        print("[Stage] 모든 Stage 완료")
        return true
    end

    state.currentStage = nextStage
    player:ResetMovement()
    player.Velocity = FVector(0, 0, 0)
    _G.RespawnLocation = nextLocation
    player.Location = nextLocation

    print("[Stage] " .. (nextStage - 1) .. " -> " .. nextStage)
    return true
end

function BeginPlay()
    state.currentStage = state.currentStage or 1
    _G.StageManager = { Advance = Advance }
    _G.Stage = {
    current = 1,
    version = 0,
    starts = stageStarts,
    }
end

