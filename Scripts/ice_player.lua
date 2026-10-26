-- SlidingMovementComponent가 있는 플레이어에 연결한다. W/S: X축, A/D: Y축.
local RESPAWN_DELAY = 1.0
local player = script_state -- 이 플레이어의 사망 상태와 시작 위치. 핫 리로드에도 유지한다.
local targetLocation = FVector(0, 0, 0)
local targetDirection = FVector(0, 0, 0)
local bIsMoving = false
local bWasOnIce = false

local function InitMovementState()
    targetLocation = FVector(0, 0, 0)
    bIsMoving = false
end

local function Respawn()
    obj:ResetMovement()
    obj.Location = player.spawnLocation -- 부활할 때만 시작 위치로 텔레포트한다.
    player.isDead = false
    player.respawnTime = 0
    obj:SetMovementEnabled(true)
    InitMovementState()
end

local function Die(wall)
    if player.isDead then return end
    player.isDead = true
    player.respawnTime = RESPAWN_DELAY
    obj:SetMovementEnabled(false)

    -- 목숨 감소, 사망 연출, 게임 오버 같은 규칙도 여기서 처리한다.
    local cause = wall and wall:IsValid() and wall.Name or "알 수 없는 충돌"
    print("[Ice] 벽 충돌로 사망: " .. cause)
end

local function UpdateIceState()
    -- World가 FCollision::Overlap으로 관리한 결과를 사용한다.
    -- 한 액터의 여러 도형 또는 여러 빙판이 겹쳐도 하나라도 남으면 빙판이다.
    for _, other in ipairs(obj:GetOverlappingActors()) do
        if other:IsValid() and other:HasComponent("UIceVolumeComponent") then
            obj:SetOnIce(true)
            return
        end
    end
    obj:SetOnIce(false)
end


function BeginPlay()
    player.spawnLocation = obj.Location
    player.isDead = false
    player.respawnTime = 0
    obj:SetMovementEnabled(true)
    UpdateIceState()
    print("[Ice] WASD: 이동 / R: 시작 위치로 복귀")

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

    if player.isDead then
        player.respawnTime = player.respawnTime - dt
        if player.respawnTime <= 0 then Respawn() end
        return
    end

    local bIsOnIce = obj.IsOnIce
    -- 얼음 위에 있다가 밖으로 나가면 정지한다
    if bWasOnIce and not bIsOnIce then
        obj:SetMoveInput(FVector(0, 0, 0))
        targetDirection = FVector(0, 0, 0)
        targetLocation = obj.Location
        bIsMoving = false
        print("[ICE] Obj got out from the ice")
        bWasOnIce = bIsOnIce
    end
    bWasOnIce = bIsOnIce

    -- 얼음위가 아니면 매 프레임 도착지점까지 이동 입력을 준다
    if not obj.IsOnIce and bIsMoving then
        local direction = (targetLocation - obj.Location)
        if direction:Length() > 0.1 then
            obj:SetMoveInput(direction)
        else
            -- 남은 거리가 0.1 미만이면 정지
            obj:SetMoveInput(FVector(0, 0, 0))
            bIsMoving = false
        end
    elseif bIsMoving then
        -- 얼음 위면 처음 지정한 방향 그대로 간다
        obj:SetMoveInput(targetDirection)
    else -- bIsMoving == false
        obj:SetMoveInput(FVector(0, 0, 0))
    end

    -- 실제 이동 방향으로 회전한다.
    local steeringDirection = obj:GetSteeringDirection()
    -- 방향이 0이면 회전하지 않는다
    if steeringDirection:Length() > 0.01 then
        local rotation = obj.Rotation
        rotation:LookDirection(steeringDirection)
        obj.Rotation = rotation
    end
end

function OnHit(wall)
    Die(wall) -- 벽 충돌을 사망으로 해석하는 규칙은 Lua에 둔다.
end

local function OnRightMousePressed()
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
        targetDirection = (targetLocation - obj.Location)
        bIsMoving = true;
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