#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "MeetingButton.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

/**
 * 긴급 소집 벨. 레벨에 배치 후 열차의 자식으로 붙인다.
 * 사용 조건(게임 시작 1분 후 + 직전 회의 종료 1분 쿨다운)은 서버 GameMode 가 판정하고,
 * 프롬프트 표시는 GameState.bMeetingBellReady (복제) 를 읽는다.
 */
UCLASS()
class TEAM4PROJECT_API AMeetingButton : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AMeetingButton();

	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meeting")
	TObjectPtr<UStaticMeshComponent> ButtonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Meeting")
	TObjectPtr<UBoxComponent> InteractionBox;
};
