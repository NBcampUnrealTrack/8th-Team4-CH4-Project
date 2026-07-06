#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "DoorBase.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UDataTable;

UCLASS()
class TEAM4PROJECT_API ADoorBase : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ADoorBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ============================================================
	// 컴포넌트
	// ============================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DoorFrame;

	/** 실제로 회전하는 문 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DoorMesh;

	/** 플레이어가 근접 인터랙션할 수 있는 범위 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* InteractionVolume;

	// ============================================================
	// 복제 상태
	// ============================================================
	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "Door")
	bool bIsOpen;

	/** 마피아 마스터키로 잠금. 잠긴 동안에는 일반 인터랙션 불가 */
	UPROPERTY(ReplicatedUsing = OnRep_IsLocked, BlueprintReadOnly, Category = "Door")
	bool bIsLocked;

	// ============================================================
	// 서버 전용 인터랙션 API
	// void AMyCharacter::Server_InteractDoor_Implementation() { NearbyDoor->ToggleDoor(); }
	// ============================================================

	/** 문 열기/닫기 토글.*/
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ToggleDoor();

	/** 마피아 전용 잠금/해제. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetLocked(bool bLock);

	// IInteractable
	virtual void Interact_Implementation(ACharacter* Interactor) override;
	virtual FText GetInteractPrompt_Implementation() const override;

	// ============================================================
	// 에디터 설정
	// ============================================================
	/** 열릴 때 회전하는 Yaw 각도 (도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Config")
	float OpenAngleDegrees = 90.f;

	/** 문 회전 속도 (도/초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Config")
	float OpenSpeed = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Config")
	bool bStartOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Config")
	bool bStartLocked = false;

	/** 문 사운드 DT (캐릭터 사운드 DT 공용 — Door.Open / Door.Close 행 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Config")
	TObjectPtr<UDataTable> SoundTable;

protected:
	UFUNCTION()
	void OnRep_IsOpen();

	UFUNCTION()
	void OnRep_IsLocked();

private:
	FRotator ClosedRotation;
	FRotator OpenRotation;
	FRotator TargetRotation;
	bool bIsAnimating = false;
	bool bRotationsInitialized = false;

	// 마지막으로 사운드를 재생한 개폐 상태 — 초기 복제(bStartOpen)로 인한 시작 시 소리 방지.
	bool bLastOpenStateForSound = false;
};
