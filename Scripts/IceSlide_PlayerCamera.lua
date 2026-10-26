-- 액터마다 별도 environment에서 실행된다. obj는 이 스크립트가 붙은 액터다.
local cameraSpeed = 15
local EDGE_THRESHOLD = 0.1

_G.GameEvents = _G.GameEvents or {}

local function MoveToPlayer()
    obj.Location = playerActor.Location + initialPosition
end

function BeginPlay()
    print("[BeginPlay] " .. obj.UUID)

    playerActor = GetActorByName("IcePlayer")
    if not playerActor:IsValid() then
        print("[BeginPlay] Error: playerActor is not valid")
    else
        print("[BeginPlay] Actor found. Name: " .. playerActor.Name .. "UUID: " .. playerActor.UUID)
    end
    initialPosition = obj.Location - playerActor.Location
    print("[BeginPlay] Initial Position: ", initialPosition)

    _G.GameEvents.ResetCamera = function()
        MoveToPlayer()
    end
end

function EndPlay()
    print("[EndPlay] " .. obj.UUID)
    if _G.GameEvents then
        _G.GameEvents.ResetCamera = nil
    end
end

function OnBeginOverlap(OtherActor)
    print("[BeginOverlap] " .. OtherActor.UUID)
end

function OnEndOverlap(OtherActor)
end

local function MoveLeft(dt)
    obj.Location = obj.Location + FVector(0, -cameraSpeed, 0) * dt
end

local function MoveRight(dt)
    obj.Location = obj.Location + FVector(0, cameraSpeed, 0) * dt
end

local function MoveForward(dt)
    obj.Location = obj.Location + FVector(cameraSpeed, 0, 0) * dt
end

local function MoveBack(dt)
    obj.Location = obj.Location + FVector(-cameraSpeed, 0, 0) * dt
end

function Tick(dt)
    if IsKeyDown('w') then
        MoveForward(dt)
    end
    if IsKeyDown('a') then
        MoveLeft(dt)
    end
    if IsKeyDown('s') then
        MoveBack(dt)
    end
    if IsKeyDown('d') then
        MoveRight(dt)
    end
    if IsKeyDown('space') then
        MoveToPlayer()
    end

    local viewportX, viewportY = GetViewportSize()
    local mousePositionX, mousePositionY = GetMousePosition()

    local normalizedMouseX = mousePositionX / viewportX
    local normalizedMouseY = mousePositionY / viewportY

    if normalizedMouseX < EDGE_THRESHOLD then
        MoveLeft(dt)
    end
    if normalizedMouseX > 1 - EDGE_THRESHOLD then
        MoveRight(dt)
    end
    if normalizedMouseY < EDGE_THRESHOLD then
        MoveForward(dt)
    end
    if normalizedMouseY > 1 - EDGE_THRESHOLD then
        MoveBack(dt)
    end


end
