-- SlidingMovementComponent가 있는 플레이어에 연결한다. W/S: X축, A/D: Y축.
local respawnTime = 0

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
    UpdateIceState()
    print("[Ice] WASD: 이동 / R: 시작 위치로 복귀")
end

function OnReload()
    respawnTime = 0
    obj:Respawn()
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
        respawnTime = 0
        obj:Respawn()
        return
    end
    if obj.IsDead then
        respawnTime = respawnTime - dt
        if respawnTime <= 0 then obj:Respawn() end
        return
    end
    local x, y = 0, 0
    if IsKeyDown("w") then x = x + 1 end
    if IsKeyDown("s") then x = x - 1 end
    if IsKeyDown("d") then y = y + 1 end
    if IsKeyDown("a") then y = y - 1 end
    obj:SetMoveInput(FVector(x, y, 0))
end

function OnDeath(wall)
    print("[Ice] 벽 충돌로 사망: " .. wall.Name)
    respawnTime = 1.0
end
