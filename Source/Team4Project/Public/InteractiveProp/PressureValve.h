#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "PressureValve.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPressureComponent;
class ABaseCharacter;

UCLASS()
class TEAM4PROJECT_API APressureValve : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	APressureValve();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ValveMesh;

	// 플레이어가 인터랙션할 수 있는 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBox;

	// 에디터에서 GODTrain Actor를 드래그해서 연결. 비워두면 부모 액터(Child Actor Component로
	// GODTrain 밑에 붙인 경우 등)에서 자동 탐색한다 (CoalFeeder::GetFurnace()와 동일한 패턴).
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Valve")
	AActor* TrainActor;

	// 미니게임 진행 중 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Valve")
	bool bMinigameActive = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Valve")
	int32 SuccessCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Valve")
	int32 MissCount = 0;

	// 현재 라운드(바늘 왕복) 시작 시각 (서버 월드시간)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Valve")
	float RoundStartServerTime = 0.f;

	// 1회 시도 제한시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float RoundDuration = 8.f;

	// 바늘이 왕복 1회 완주하는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float NeedleOscillationPeriod = 2.f;

	// 정답 구간 (0~1 정규화)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float RedZoneStart = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float RedZoneEnd = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	int32 RequiredSuccesses = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	int32 MaxMisses = 3;

	// 1회 성공 시 감압량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float PressureReductionPerSuccess = 20.f;

	// 경과시간 기반 바늘 위치(0~1, 왕복 삼각파). 서버 판정과 위젯 표시가 동일 공식을 공유.
	static float ComputeNeedlePosition(float Elapsed, float Period);

	// BaseCharacter가 스페이스 입력을 relay할 때 호출 (서버 전용)
	void SubmitStopInput();

	// 화부가 강제 차단 시 진행 중인 미니게임을 즉시 중단 (서버 전용)
	void ForceStop();

	// 미니게임 플레이어 사망/이탈 시: 실패(강제 폭발) 없이 중단 (서버 전용)
	void AbortMinigame();

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

private:
	void StartMinigame(ABaseCharacter* Player);
	void StartNextRound();
	void OnRoundTimeout();
	void EvaluateRoundResult();

	// MinigamePlayer 를 비우고 사망 델리게이트를 언바인딩한 뒤 반환 (모든 종료 경로 공용).
	ABaseCharacter* ReleaseMinigamePlayer();

	UFUNCTION()
	void OnMinigamePlayerDied(ABaseCharacter* DeadCharacter, AActor* Killer);

	UPressureComponent* GetPressureComponent() const;

	FTimerHandle RoundTimeoutHandle;

	UPROPERTY()
	TObjectPtr<ABaseCharacter> MinigamePlayer;
};
