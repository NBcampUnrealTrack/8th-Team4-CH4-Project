#include "InteractiveProp/GODTrainTrack.h"
#include "Components/SplineComponent.h"

AGODTrainTrack::AGODTrainTrack()
{
	PrimaryActorTick.bCanEverTick = false;
	// 스플라인 데이터는 레벨 배치라 모든 머신에 이미 존재한다.
	// 단, GODTrain의 Track 포인터가 클라이언트에서 해석되려면(NetGUID)
	// 이 액터가 복제 가능해야 한다. 트랜스폼/움직임은 복제하지 않는다.
	bReplicates = true;
	SetReplicateMovement(false);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;

	// 닫힌 루프: 마지막 점이 첫 점과 자동 연결되어 무한 순환 가능
	Spline->SetClosedLoop(true);
}
