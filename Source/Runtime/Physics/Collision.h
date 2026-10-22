#pragma once

class UShapeComponent;

// 도형 간 교차 판정. 담당: D, 소비자: C (UWorld::UpdateOverlaps)
namespace FCollision
{
	// 두 도형이 겹치면 true. 둘 다 월드 변환이 반영된 상태로 판정한다.
	// 지원하지 않는 조합은 false
	bool Overlap(const UShapeComponent* A, const UShapeComponent* B);
}
