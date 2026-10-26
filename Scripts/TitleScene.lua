-- 액터마다 별도 environment에서 실행된다. obj는 이 스크립트가 붙은 액터다.

local titleWidget

local function GameStart()
    print("Game Start")
    OpenLevel("IceSlide")
end

function BeginPlay()
    print("[BeginPlay] " .. obj.UUID)

    titleWidget = CreateWidget("Assets/UI/TitleScene.widget")
    titleWidget:AddToViewport()
    titleWidget:Find("StartButton"):BindClick(GameStart)
end

function EndPlay()
    print("[EndPlay] " .. obj.UUID)
end

function OnBeginOverlap(OtherActor)
    print("[BeginOverlap] " .. OtherActor.UUID)
end

function OnEndOverlap(OtherActor)
end

function Tick(dt)
end
