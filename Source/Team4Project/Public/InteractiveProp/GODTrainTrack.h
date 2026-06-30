#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GODTrainTrack.generated.h"

class USplineComponent;

// 기차가 따라 달리는 정적인 루프 트랙.
// 레벨에 한 번 배치하고 GODTrain에서 참조한다.
// 스플라인 데이터는 모든 머신에 존재하고, 위치는 복제된 스칼라로 각자 재구성한다.
UCLASS()
class TEAM4PROJECT_API AGODTrainTrack : public AActor
{
	GENERATED_BODY()

public:
	AGODTrainTrack();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Track")
	USplineComponent* Spline;
};
