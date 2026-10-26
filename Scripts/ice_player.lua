-- SlidingMovementComponent가 있는 플레이어에 연결한다. W/S: X축, A/D: Y축.
local RESPAWN_DELAY = 1.0
local GROUND_MOVE_SPEED <const> = 3.0
local ICE_MOVE_SPEED <const> = 6.0
local player = script_state -- 이 플레이어의 사망 상태와 시작 위치. 핫 리로드에도 유지한다.
local targetLocation = FVector(0, 0, 0)
local targetDirection = FVector(0, 0, 0)
_G._G.bIsMoving = false
local bWasOnIce = false
local currentIceType = "ground"

local restartWidget = nil

local function InitMovementState()
    targetLocation = FVector(0, 0, 0)
    _G._G.bIsMoving = false

    local events = _G.GameEvents
    if events and events.ResetCamera then
        events.ResetCamera()
    end
end

local function Respawn()
    obj:ResetMovement()
    obj.Location = _G.RespawnLocation or player.spawnLocation
    player.isDead = false
    player.respawnTime = 0

    -- 부활 시 뒤집힌 상태를 원상복귀
    if player.aliveRotation then
    obj.Rotation = player.aliveRotation
    end

    obj:SetMovementEnabled(true)
    InitMovementState()

    -- 리스타트 위젯 숨기기
    if restartWidget and restartWidget:IsValid() then
        restartWidget.Visible = false
    end
end

-- 개발자 테스트용: 가장 가까운 StageN_Start를 현재 Stage로 보고 앞/뒤 Stage 시작 위치로 옮긴다.
local function MoveStage(offset)
    local current, nearest = nil, math.huge
    local index = 1
    local start = GetActorByName("Stage1_Start")
    while start do
        local distance = (start.Location - obj.Location):Length()
        if distance < nearest then current, nearest = index, distance end
        index = index + 1
        start = GetActorByName("Stage" .. index .. "_Start")
    end
    local target = current and GetActorByName("Stage" .. (current + offset) .. "_Start")
    if not target then return end

    _G.RespawnLocation = target.Location + FVector(0, 0, 0.5)
    Respawn()
    print("[Dev] Stage " .. current .. " -> " .. (current + offset))
end

local function Die(wall)
    if player.isDead then return end
    player.isDead = true
    player.respawnTime = RESPAWN_DELAY

    -- 사망 시 뒤집기
    player.aliveRotation = obj.Rotation
    local rotation = obj.Rotation
    rotation.roll = rotation.roll + 180
    obj.Rotation = rotation
    obj:SetMovementEnabled(false)

    -- 기존 ParticleActor의 아틀라스를 바꿔 사망 위치에서 한 번만 재생한다.
    local effect = GetActorByName("DeathParticle")
    if effect and effect:HasComponent("UParticleSubUVComponent")
        and effect:SetParticleTexture("Assets/SubUV/GreenBlood.png") then
        effect:SetSubUVSize(4, 4)
        effect:SetParticleFrameRate(60)
        effect.Location = obj.Location
        effect:PlayParticle(3.0) -- 기본 크기의 3배
    end

    -- 목숨 감소, 사망 연출, 게임 오버 같은 규칙도 여기서 처리한다.
    local cause = wall and wall:IsValid() and wall.Name or "알 수 없는 충돌"
    print("[Ice] 벽 충돌로 사망: " .. cause)

    -- 리스타트 위젯 띄우기
    if restartWidget and restartWidget:IsValid() then
        restartWidget.Visible = true
    end
end

local function UpdateIceState()
    -- World가 FCollision::Overlap으로 관리한 결과를 사용한다.
    -- 한 액터의 여러 도형 또는 여러 빙판이 겹쳐도 하나라도 남으면 빙판이다.
    local nextIceType = obj:GetOverlappingIceType() -- nil, "default", "slippery", "reverse"

    -- 이동 컴포넌트에는 기존처럼 얼음 여부를 전달한다
    obj:SetOnIce(nextIceType ~= nil)

    -- 종류가 같으면 진입 처리를 반복하지 않는다
    if nextIceType == currentIceType then
        return
    end

    local previousIceType = currentIceType
    currentIceType = nextIceType

    print(
        "[Ice] "
        .. tostring(previousIceType)
        .. " -> "
        .. tostring(currentIceType)
    )

    obj:SetMoveSpeed(ICE_MOVE_SPEED)
    if currentIceType == "default" then
        print("[Ice] Entered default ice")
    elseif currentIceType == "slippery" then
        print("[Ice] Entered slippery ice")
    elseif currentIceType == "reverse" then
        print("[Ice] Entered reverse ice")
    elseif currentIceType == nil then
        print("[Ice] Exited ice")
        obj:SetMoveSpeed(GROUND_MOVE_SPEED)
    else
        print("[Ice] Entered unknown ice type: " .. tostring(currentIceType))
    end
end

local function GoToTitle()
    OpenLevel("TitleScene")
end

function BeginPlay()
    _G.RespawnLocation = nil
    player.spawnLocation = obj.Location
    player.isDead = false
    player.respawnTime = 0

    restartWidget = CreateWidget("Assets/UI/RestartWidget.widget")
    restartWidget.Visible = false
    restartWidget:AddToViewport()
    restartWidget:Find("RestartButton"):BindClick(function()
        Respawn()
    end)
    restartWidget:Find("TitleButton"):BindClick(GoToTitle)

    obj:SetMovementEnabled(true)
    UpdateIceState()
    print("[Ice] WASD: 카메라이동 / R: 시작 위치로 복귀")

    InitMovementState()
    StartCoroutine(MouseInputCoroutine)
end

function OnReload()
    if not player.spawnLocation then player.spawnLocation = obj.Location end
    Respawn()
    UpdateIceState()
end

function OnBeginOverlap(other)
    UpdateIceState()
end

function OnEndOverlap(other)
    UpdateIceState()
end

function Tick(dt)
    if IsKeyPressed("r") then
        Respawn()
        return
    end
    if IsKeyPressed("q") then MoveStage(-1) return end
    if IsKeyPressed("e") then MoveStage(1) return end

    -- 자동 리스폰 비활성화
    -- if player.isDead then
    --     player.respawnTime = player.respawnTime - dt
    --     if player.respawnTime <= 0 then Respawn() end
    --     return
    -- end

    local bIsOnIce = obj.IsOnIce
    -- 얼음 위에 있다가 밖으로 나가면 정지한다
    if bWasOnIce and not bIsOnIce then
        obj:SetMoveInput(FVector(0, 0, 0))
        targetDirection = FVector(0, 0, 0)
        targetLocation = obj.Location
        _G.bIsMoving = false
        print("[ICE] Obj got out from the ice")
        bWasOnIce = bIsOnIce
    end
    bWasOnIce = bIsOnIce

    -- 얼음위가 아니면 매 프레임 도착지점까지 이동 입력을 준다
    if not obj.IsOnIce and _G.bIsMoving then
        local direction = (targetLocation - obj.Location)
        if direction:Length() > 0.1 then
            obj:SetMoveInput(direction:Normalize())
        else
            -- 남은 거리가 0.1 미만이면 정지
            obj:SetMoveInput(FVector(0, 0, 0))
            _G.bIsMoving = false
            print("Actor reached to the target location")
        end
    elseif _G.bIsMoving then
        -- 얼음 위면 처음 지정한 방향 그대로 간다

        obj:SetMoveInput(targetDirection:Normalize())
    else -- bIsMoving == false
        obj:SetMoveInput(FVector(0, 0, 0))
    end

    -- 실제 이동 방향으로 회전한다.
    local steeringDirection = obj:GetSteeringDirection()
    -- 방향이 0이면 회전하지 않는다
    if steeringDirection:Length() > 0.01 then
        local rotation = obj.Rotation
        if currentIceType == "reverse" then
            rotation:LookDirection(-steeringDirection)
        else
            rotation:LookDirection(steeringDirection)
        end
        obj.Rotation = rotation
    end
end

function OnHit(wall)
    Die(wall) -- 벽 충돌을 사망으로 해석하는 규칙은 Lua에 둔다.
end

local function OnRightMousePressed()
    if currentIceType == "slippery" then
        print("Mouse input is disabled on the slippery ice")
        return
    end


    local x, y = GetMousePosition()
    print("[OnRightMousePressed] " .. x, y)

    local origin, direction = ScreenToWorldRay(x, y)
    if origin then
        print("start: ", origin)
        print("direction: ", direction)
    end

    local hitPosition = Raycast(origin, direction)
    if hitPosition then
        print("hit world position: ",
            hitPosition.x,
            hitPosition.y,
            hitPosition.z)

        print("start move")
            
        targetLocation = FVector(hitPosition.x, hitPosition.y, obj.Location.z)
        if currentIceType == "default" then
            targetDirection = (targetLocation - obj.Location)
        elseif currentIceType == "reverse" then
            -- 얼음 타입이 reverse이면 마우스 클릭의 반대 방향으로 간다
            targetDirection = (obj.Location - targetLocation)
        end
        _G.bIsMoving = true;
        print("target location: ", targetLocation)
        print("target direction: ", targetDirection)
        --move_to(targetLocation, 10)
    end
end

function MouseInputCoroutine()
    while true do
        wait_until(function()
            return IsRightMouseButtonPressed()
        end)

        OnRightMousePressed()
    end
end
