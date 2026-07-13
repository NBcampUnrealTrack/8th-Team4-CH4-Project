#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeetingRoom.generated.h"

class ACharacter;

/**
 * 긴급 회의 소환 지점. 레벨에 하나 배치하고 열차의 자식으로 붙인다 (스플라인 이동).
 * 좌석은 액터 기준 상대 오프셋 — 기본값은 원형 8석. 에디터에서 자유롭게 수정.
 */
UCLASS()
class TEAM4PROJECT_API AMeetingRoom : public AActor
{
	GENERATED_BODY()

public:
	AMeetingRoom();

	// SeatIndex 번째 좌석으로 텔레포트 (서버). 좌석보다 인원이 많으면 순환.
	void TeleportToSeat(ACharacter* Character, int32 SeatIndex) const;

	// 좌석 월드 트랜스폼. 회전은 방 중앙을 바라본다 (토론 대형).
	FTransform GetSeatTransform(int32 SeatIndex) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meeting")
	TObjectPtr<USceneComponent> Root;

	// 좌석 상대 오프셋 (액터 기준). 생성자가 원형 8석을 채워 둔다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meeting")
	TArray<FVector> SeatOffsets;

	// 기본 좌석 원의 반지름 (cm). SeatOffsets 를 직접 수정하면 무시된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meeting")
	float DefaultSeatRadius = 250.f;
};
