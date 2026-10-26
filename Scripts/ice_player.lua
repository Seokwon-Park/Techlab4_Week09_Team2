-- SlidingMovementComponent가 있는 플레이어에 연결한다. W/S: X축, A/D: Y축.
local respawnTime = 0
local targetLocation = FVector(0, 0, 0)
local targetDirection = FVector(0, 0, 0)
local bIsMoving = false
local bWasOnIce = false

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

local function InitMovementState()
    targetLocation = FVector(0, 0, 0)
    bIsMoving = false
end

local function Respawn()
    respawnTime = 0
    obj:Respawn()
    InitMovementState()
end

function BeginPlay()
    UpdateIceState()
    print("[Ice] WASD: 이동 / R: 시작 위치로 복귀")

    InitMovementState()
    StartCoroutine(MouseInputCoroutine)
end

function OnReload()
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
    if obj.IsDead then
        respawnTime = respawnTime - dt
        if respawnTime <= 0 then 
            Respawn()
        end
        return
    end
    -- local x, y = 0, 0
    -- if IsKeyDown("w") then x = x + 1 end
    -- if IsKeyDown("s") then x = x - 1 end
    -- if IsKeyDown("d") then y = y + 1 end
    -- if IsKeyDown("a") then y = y - 1 end

    local bIsOnIce = obj.IsOnIce
    -- 얼음 위에 있다가 밖으로 나가면 정지한다
    if bWasOnIce and not bIsOnIce then
        obj:SetMoveInput(FVector(0, 0, 0))
        targetDirection = FVector(0, 0, 0)
        targetLocation = obj.Location
        bIsMoving = false
        print("[ICE] Obj got out from the ice")
        bWasOnIce = bIsOnIce
        return
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

function OnDeath(wall)
    print("[Ice] 벽 충돌로 사망: " .. wall.Name)
    respawnTime = 1.0
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