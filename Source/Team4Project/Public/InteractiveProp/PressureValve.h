#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressureValve.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPressureComponent;

UCLASS()
class TEAM4PROJECT_API APressureValve : public AActor
{
	GENERATED_BODY()

public:
	APressureValve();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ValveMesh;

	// 플레이어가 인터랙션할 수 있는 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionBox;

	// 밸브 회전 시각화 (복제 → 모든 클라이언트 동기화)
	UPROPERTY(ReplicatedUsing = OnRep_ValveRotation, BlueprintReadOnly, Category = "Valve")
	float ValveRotation = 0.f;

	// 조작 중 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Valve")
	bool bIsTurning = false;

	// 초당 감압량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	float ReducePerSecond = 10.f;

	// 에디터에서 GODTrain Actor를 드래그해서 연결
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Valve")
	AActor* TrainActor;

	// 밸브 조작 시작 (클라이언트 → 서버)
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Valve")
	void Server_StartTurning(AController* Operator);

	// 밸브 조작 중지
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Valve")
	void Server_StopTurning();

private:
	UFUNCTION() void OnRep_ValveRotation();

	UPressureComponent* GetPressureComponent() const;
};
