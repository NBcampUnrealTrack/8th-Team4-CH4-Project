#include "Meeting/MeetingRoom.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AMeetingRoom::AMeetingRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 기본 좌석: 원형 8석 (세션 정원과 동일).
	for (int32 i = 0; i < 8; ++i)
	{
		const float Rad = 2.f * PI * i / 8.f;
		SeatOffsets.Add(FVector(FMath::Cos(Rad) * DefaultSeatRadius,
		                        FMath::Sin(Rad) * DefaultSeatRadius, 0.f));
	}
}

FTransform AMeetingRoom::GetSeatTransform(int32 SeatIndex) const
{
	if (SeatOffsets.Num() == 0)
	{
		return GetActorTransform();
	}

	const FVector Offset   = SeatOffsets[SeatIndex % SeatOffsets.Num()];
	const FVector WorldLoc = GetActorTransform().TransformPosition(Offset);

	// 방 중앙을 바라보게. 열차가 기울어 있어도 캐릭터는 yaw 만 쓴다.
	const FRotator LookAtCenter = (-GetActorTransform().TransformVectorNoScale(Offset)).Rotation();
	return FTransform(FRotator(0.f, LookAtCenter.Yaw, 0.f), WorldLoc);
}

void AMeetingRoom::TeleportToSeat(ACharacter* Character, int32 SeatIndex) const
{
	if (!Character) return;

	const FTransform Seat = GetSeatTransform(SeatIndex);

	// 낙하/이동 속도 제거 후 텔레포트 (BaseCharacter::RescueToStart 와 동일한 패턴).
	if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}

	if (!Character->TeleportTo(Seat.GetLocation(), Seat.Rotator()))
	{
		// 다른 플레이어와 겹치는 등으로 실패하면 강제 이동.
		Character->SetActorLocation(Seat.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(Seat.Rotator());
	}
}
