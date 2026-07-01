#include "InteractiveProp/CoalFeeder.h"
#include "InteractiveProp/CoalItem.h"
#include "Component/FurnanceComponent.h"
#include "Player/BaseCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACoalFeeder::ACoalFeeder()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	FeederMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FeederMesh"));
	RootComponent = FeederMesh;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(60.f, 60.f, 60.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 플레이어 InteractSphere(WorldDynamic)가 이 박스를 감지하려면, 박스가 그 채널을
	// Overlap해야 한다. 모든 채널 Overlap + 객체타입 WorldDynamic으로 확실히 감지되게 한다.
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBox->SetGenerateOverlapEvents(true);

	// 연료 상태 3D 위젯. Widget Class(WBP)는 BP 디테일에서 지정한다.
	FuelWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("FuelWidget"));
	FuelWidget->SetupAttachment(RootComponent);
	FuelWidget->SetWidgetSpace(EWidgetSpace::World);
	FuelWidget->SetDrawSize(FVector2D(200.f, 60.f));
	FuelWidget->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	FuelWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACoalFeeder::BeginPlay()
{
	Super::BeginPlay();

	// 기차에 부착 → 기차가 스플라인 따라 움직일 때 투입구도 함께 이동.
	// 배치한 위치를 유지(KeepWorldTransform)한 채 기차의 자식이 된다.
	if (bAttachToTrain && TrainActor)
	{
		AttachToActor(TrainActor, FAttachmentTransformRules::KeepWorldTransform);
	}

	// 화로 연료 변화에 위젯을 연동. 클라이언트에서 기차/화로 복제가 늦으면
	// 타이머로 몇 차례 재시도한다(성공 시 타이머 정리).
	if (!TryBindFurnace())
	{
		GetWorldTimerManager().SetTimer(
			FurnaceBindTimer, this, &ACoalFeeder::RetryBindFurnace, 0.5f, /*bLoop=*/true);
	}
}

bool ACoalFeeder::TryBindFurnace()
{
	if (BoundFurnace)
	{
		return true; // 이미 바인딩됨
	}

	UFurnanceComponent* Furnace = GetFurnace();
	if (!Furnace)
	{
		return false;
	}

	BoundFurnace = Furnace;
	Furnace->OnFuelLevelChanged.AddDynamic(this, &ACoalFeeder::HandleFuelLevelChanged);

	// 현재 값으로 즉시 한 번 표시(델리게이트는 다음 변화 때까지 안 오므로)
	RefreshFuelWidget();
	return true;
}

void ACoalFeeder::RetryBindFurnace()
{
	if (TryBindFurnace())
	{
		GetWorldTimerManager().ClearTimer(FurnaceBindTimer);
	}
}

void ACoalFeeder::HandleFuelLevelChanged(float /*FuelPercent*/)
{
	// 화로에서 절대값을 직접 읽어 위젯 이벤트로 넘긴다.
	RefreshFuelWidget();
}

void ACoalFeeder::RefreshFuelWidget()
{
	UFurnanceComponent* Furnace = BoundFurnace ? BoundFurnace : GetFurnace();
	if (!Furnace)
	{
		return;
	}

	// 숫자 표시는 소수점 없이(정수). 게이지 채움용 Percent는 부드러움을 위해 실수 유지.
	const float Cur = FMath::RoundToFloat(Furnace->CurrentFuel);
	const float Max = FMath::RoundToFloat(Furnace->MaxFuel);
	const float Percent = (Furnace->MaxFuel > 0.f)
		? FMath::Clamp(Furnace->CurrentFuel / Furnace->MaxFuel, 0.f, 1.f)
		: 0.f;

	OnFuelWidgetUpdate(Cur, Max, Percent);
}

UFurnanceComponent* ACoalFeeder::GetFurnace() const
{
	// 1) 디테일에서 명시적으로 지정한 TrainActor 우선
	if (TrainActor)
	{
		if (UFurnanceComponent* F = TrainActor->FindComponentByClass<UFurnanceComponent>())
		{
			return F;
		}
	}

	// 2) 미지정이면, 부모 액터에서 자동 탐색
	//    (레벨에서 기차 밑 자식으로 넣었거나 Child Actor로 붙인 경우)
	AActor* Parent = GetAttachParentActor();
	if (!Parent)
	{
		Parent = GetParentActor();
	}
	for (; Parent; Parent = Parent->GetAttachParentActor())
	{
		if (UFurnanceComponent* F = Parent->FindComponentByClass<UFurnanceComponent>())
		{
			return F;
		}
	}

	return nullptr;
}

void ACoalFeeder::Interact_Implementation(ACharacter* Interactor)
{
	// Interact 는 서버에서 호출된다(InteractComponent::Server_TryInteract).
	UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] Interact 호출됨 (Authority=%d)"), HasAuthority());
	if (!HasAuthority()) return;

	ABaseCharacter* BaseChar = Cast<ABaseCharacter>(Interactor);
	if (!BaseChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] 실패: Interactor가 BaseCharacter 아님"));
		return;
	}

	UFurnanceComponent* Furnace = GetFurnace();
	if (!Furnace)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] 실패: Furnace 없음 (TrainActor=%s)"),
			*GetNameSafe(TrainActor));
		return;
	}

	// 화로가 가득 차 있으면 투입 불가 (석탄 낭비 방지)
	if (Furnace->CurrentFuel >= Furnace->MaxFuel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] 실패: 화로 가득 참 (%.1f/%.1f)"),
			Furnace->CurrentFuel, Furnace->MaxFuel);
		return;
	}

	// 손에 든 게 석탄일 때만 투입
	ACoalItem* Coal = Cast<ACoalItem>(BaseChar->GetCurrentHeldItem());
	if (!Coal)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] 실패: 손에 든 석탄 없음 (HeldItem=%s)"),
			*GetNameSafe(BaseChar->GetCurrentHeldItem()));
		return;
	}

	// 쿨다운 체크 (연속 투입 방지)
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFeedTime < FeedCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] 실패: 쿨다운 중"));
		return;
	}
	LastFeedTime = Now;

	UE_LOG(LogTemp, Warning, TEXT("[CoalFeeder] 성공: 석탄 투입 (+%.1f)"), Coal->FuelValue);

	// 연료 투입 + 이펙트/소리 (전 클라)
	Furnace->AddCoal(Coal->FuelValue);
	Multicast_PlayFeedFX();

	// 석탄 소모: 장착 태그/슬롯/무게/부착 정리 → 손 비주얼 해제 → 액터 제거
	Coal->Server_Drop();
	BaseChar->SetCoalEquipped(false);
	Coal->Destroy();
}

void ACoalFeeder::Multicast_PlayFeedFX_Implementation()
{
	// 데디케이티드 서버는 시각/청각 연출 생략
	if (GetNetMode() == NM_DedicatedServer) return;

	const FVector Loc = GetActorTransform().TransformPosition(EffectOffset);

	if (FeedEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), FeedEffect, Loc);
	}
	if (FeedSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FeedSound, Loc);
	}
}

FText ACoalFeeder::GetInteractPrompt_Implementation() const
{
	// 화로가 가득이면 안내 (CurrentFuel 은 복제되어 클라에서도 읽힘)
	if (UFurnanceComponent* Furnace = GetFurnace())
	{
		if (Furnace->CurrentFuel >= Furnace->MaxFuel)
		{
			return FText::FromString(TEXT("화로 가득 참"));
		}
	}
	return FText::FromString(TEXT("석탄 넣기"));
}
