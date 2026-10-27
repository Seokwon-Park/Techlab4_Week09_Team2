#pragma once

#include "LightComponent.h"
#include "DirectionalLightComponent.generated.h"

UCLASS()
class UDirectionalLightComponent :public ULightComponent
{
	GENERATED_BODY()
public:
	UDirectionalLightComponent();
	virtual ~UDirectionalLightComponent() override;

	virtual class FLightSceneProxy* CreateSceneProxy() const override;

	ELightType GetLightType() const override { return ELightType::Directional; }
	float GetAngularRadiusDegrees() const { return AngularRadiusDegrees; }
	void SetAngularRadiusDegrees(float InDegrees);
	void PostEditChangeProperty(const FProperty& Property) override;


private:
	UPROPERTY(EditAnywhere)
	float AngularRadiusDegrees = 3.0f;
};
