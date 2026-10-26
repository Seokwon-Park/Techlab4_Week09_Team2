-- SlidingMovementComponent가 있는 플레이어에 연결한다. W/S: X축, A/D: Y축.
local RESPAWN_DELAY = 1.0
local player = script_state -- 이 플레이어의 사망 상태와 시작 위치. 핫 리로드에도 유지한다.

local function Respawn()
    obj:ResetMovement()
    obj.Location = player.spawnLocation -- 부활할 때만 시작 위치로 텔레포트한다.
    player.isDead = false
    player.respawnTime = 0
    obj:SetMovementEnabled(true)
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
    local x, y = 0, 0
    if IsKeyDown("w") then x = x + 1 end
    if IsKeyDown("s") then x = x - 1 end
    if IsKeyDown("d") then y = y + 1 end
    if IsKeyDown("a") then y = y - 1 end
    obj:SetMoveInput(FVector(x, y, 0))
end

function OnHit(wall)
    Die(wall) -- 벽 충돌을 사망으로 해석하는 규칙은 Lua에 둔다.
end
