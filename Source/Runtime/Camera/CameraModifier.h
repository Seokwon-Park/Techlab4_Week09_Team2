#pragma once

class APlayerCameraManager;
class UCameraModifier : public UObject
{
public:

	uint8 Priority;
protected:
	APlayerCameraManager* CameraOwner;
	float AlphaInTime;
	float AlphaOutTime;
	float Alpha;
	uint32 bDisabled;
};