#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "Quest/QuestTypes.h"
#include "QuestStation.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class ABaseCharacter;

/**
 * 퀘스트 상호작용 지점 (설거지대, 배전반, 굴뚝 등).
 */
UCLASS()
class TEAM4PROJECT_API AQuestStation : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AQuestStation();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// DT_Quest 의 행 이름.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UDataTable> QuestTable;

	bool GetQuestRow(FQuestRow& OutRow) const;
	FText GetDisplayName() const;
	FText GetLocationHint() const;
	float GetMinDuration() const;
	TSubclassOf<UQuestMinigameWidget> GetWidgetClass() const;

	// 다른 사람이 이미 붙어 있는가 (한 스테이션에 동시 한 명).
	bool IsBusy() const { return QuestPlayer != nullptr; }

	// 서버: 미니게임 시작 / 중단.
	void StartQuest(ABaseCharacter* Player);
	void AbortQuest();

	// 서버: 완료 보고가 최소 소요 시간을 지켰는지.
	bool HasMinDurationElapsed() const;

	ABaseCharacter* GetQuestPlayer() const { return QuestPlayer; }
	ABaseCharacter* ReleaseQuestPlayer();

	// 지금 상호작용 가능한가 (Playing 페이즈 + 아무도 안 붙어 있음).
	bool IsUsableNow() const;

	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UStaticMeshComponent> StationMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UBoxComponent> InteractionBox;

	// 현재 이 스테이션에 붙어 있는 플레이어 (서버 권위, 프롬프트용으로 복제).
	UPROPERTY(Replicated)
	TObjectPtr<ABaseCharacter> QuestPlayer;

	UFUNCTION()
	void OnQuestPlayerDied(ABaseCharacter* DeadCharacter, AActor* Killer);

private:
	// 서버 시간. 완료 보고의 최소 소요 시간 검증에만 쓴다.
	double StartServerTime = 0.0;
};
