// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACoalHandVisualActor.generated.h"

class ACharacter;

UCLASS()
class TEAM4PROJECT_API AACoalHandVisualActor : public AActor
{
	GENERATED_BODY()

public:
	AACoalHandVisualActor();
	virtual void BeginPlay() override;

	// 무기(ABaseWeapon)와 동일한 방식: 서버에서 호출하면 소유 캐릭터 메시 소켓에 부착.
	// 서버는 즉시, 클라는 OnRep_HolderCharacter 에서 동일하게 부착한다.
	void AttachToCharacter(ACharacter* InCharacter, FName InSocketName);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	float VisualScale = 0.1f;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visual")
	UMaterialInterface* CoalMaterial;

	// 부착 대상 캐릭터(변경 시 클라에서도 메시 소켓에 부착). 소켓 이름과 함께 복제.
	UPROPERTY(ReplicatedUsing = OnRep_HolderCharacter)
	TObjectPtr<ACharacter> HolderCharacter;

	UPROPERTY(Replicated)
	FName AttachSocketName;

	UFUNCTION()
	void OnRep_HolderCharacter();

	// HolderCharacter/소켓 기준으로 실제 부착을 적용 (서버/클라 공통).
	void ApplyAttachment();
};
