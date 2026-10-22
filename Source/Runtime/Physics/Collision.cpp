#include "EnginePCH.h"
#include "Collision.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"

namespace FCollision
{
	bool Overlap(const UShapeComponent* A, const UShapeComponent* B)
	{
		if (!A || !B || A == B)
			return false;

		// TODO(D): 1일차 구-구 → 2일차 구-박스, 캡슐-구, 캡슐-캡슐, 박스-박스(OBB SAT), 캡슐-박스
		return false;
	}
}
