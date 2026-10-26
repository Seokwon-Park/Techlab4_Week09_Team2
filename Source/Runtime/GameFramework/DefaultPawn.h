#pragma once

#include "GameFramework/Pawn.h"

class UCameraComponent;
class UStaticMeshComponent;

class ADefaultPawn : public APawn
{
	DECLARE_CLASS(ADefaultPawn, APawn)
public:
	ADefaultPawn();
	void AddMovementInput(const FVector& WorldDirection, float ScaleValue) override;
	void AddControllerYawInput(float Value) override;
	void AddControllerPitchInput(float Value) override;
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }
private:
	UCameraComponent* CameraComponent = nullptr;
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	float MoveSpeed = 20.0f;
};
