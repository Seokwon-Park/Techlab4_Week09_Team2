-- SlidingMovementComponent가 있는 플레이어에 연결한다. W/S: X축, A/D: Y축.
local respawnTime = 0

function BeginPlay()
    print("[Ice] WASD: 이동 / R: 시작 위치로 복귀")
end

function OnReload()
    respawnTime = 0
    obj:Respawn()
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
