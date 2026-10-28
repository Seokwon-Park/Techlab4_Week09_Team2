#pragma once

#include "Camera/CameraModifier.h"
#include "Containers/Array.h"
#include "CameraModifier_CameraShake.generated.h"

class UCameraShakeBase;

UCLASS()
// 재생 중인 여러 Shake를 보관하고 결과를 하나의 POV에 합산합니다.
// 개별 Shake의 시간 계산은 각 인스턴스에 맡기고, 생성·중지·종료 정리를 담당합니다.
class UCameraModifier_CameraShake : public UCameraModifier
{
    GENERATED_BODY()

public:
    // 멤버 선언의 기본값으로 객체의 초기 상태를 준비합니다.
    // 재생이나 외부 연결은 별도의 시작·등록 함수에서 수행합니다.
    UCameraModifier_CameraShake();
    // RemoveAllCameraShakes(true)로 소유 인스턴스를 정리합니다.
    // Outer가 자동 소유권 해제를 제공한다고 가정하지 않습니다.
    virtual ~UCameraModifier_CameraShake() override;

    // Super로 Alpha를 갱신하고 각 Shake를 BasePOV 사본에 적용한 뒤 그 차이만 최종 POV에 더합니다.
    // 앞선 Shake가 바꾼 회전을 다음 Shake의 기준으로 쓰면 재생 순서에 따라 방향이 달라집니다.
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
    // 클래스·상속·생성 가능 여부를 검사하고 NewObject, CDO 설정 복사, StartShake를 거쳐 목록에 넣습니다.
    // 같은 클래스 두 번 재생은 서로 다른 객체이므로 시간과 종료가 독립적입니다.
    UCameraShakeBase* AddCameraShake(UClass* ShakeClass, float Scale = 1.0f);
    // 이 목록에 속한 인스턴스인지 확인하고 StopShake를 요청한 뒤 종료 항목을 정리합니다.
    // 다른 Modifier가 소유한 포인터를 잘못 중지하지 않습니다.
    void RemoveCameraShake(UCameraShakeBase* ShakeInstance, bool bImmediately = false);
    // 살아 있는 모든 소유 인스턴스에 StopShake를 요청합니다.
    // 일반 중지는 페이드 완료까지 목록에 남고 즉시 중지는 바로 정리됩니다.
    void RemoveAllCameraShakes(bool bImmediately = false);
    // 현재 재생 중인 Shake 목록을 읽기 전용으로 반환합니다.
    // 이 목록의 인스턴스는 Modifier가 소유하므로 호출자가 직접 삭제하지 않습니다.
    const TArray<UCameraShakeBase*>& GetActiveShakes() const { return ActiveShakes; }

protected:
    // 역순으로 목록을 순회해 완료된 항목을 먼저 목록에서 빼고 delete합니다.
    // 정방향 삭제 시 다음 항목을 건너뛰는 문제를 피하고, 전역 종료 시 이미 삭제된 포인터도 건너뜁니다.
    void RemoveFinishedShakes();

    // 소유 목록. 종료/파괴 시 명시적으로 해제. Outer는 자동 삭제를 보장하지 않는다.
    TArray<UCameraShakeBase*> ActiveShakes;
};
