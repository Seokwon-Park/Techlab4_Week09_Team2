#pragma once

#include "GameFramework/Actor.h"
#include "Input/InputSystem.h"
#include "PlayerController.generated.h"

class APawn;

UCLASS()
class APlayerController : public AActor
{
	GENERATED_BODY()
public:
	APlayerController();
	void Tick(float DeltaTime) override;
	void ProcessPlayerInput(float DeltaTime);
	void Possess(APawn* InPawn);
	void UnPossess();
	APawn* GetPawn() const { return PossessedPawn; }
	void InputKey(int32 Key, bool bDown);
	void InputAxis(EGameInputAxis Axis, float Delta);
	void ClearInputState();

	FRotator ControlRotation;
private:

	APawn* PossessedPawn = nullptr;
	bool bKeyDown[256]{};
	float MouseAxisX = 0.0f;
	float MouseAxisY = 0.0f;
	float LookSensitivity = 0.1f;
};
