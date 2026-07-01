// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BaseCharacter.h"
#include "Player/GODPlayerState.h"
#include "Player/Component/BaseAbilitySystemComponent.h"
#include "Player/Component/BaseAttributeSet.h"
#include "Player/Weapon/BaseWeapon.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/ItemBase.h"
#include "Game/BaseGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Game/BaseDataSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/Engine.h" // [임시] 디버그 화면 출력용

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UBaseAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));

	InteractComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractComponent"));

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); 

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true; 

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); 
	FollowCamera->bUsePawnControlRotation = false; 
	
	
	// 움직이는 기차 위에서 점프해도 발판(기차)에 계속 붙어 함께 이동 → 제자리 착지.
	// 공중에서도 기차 트랜스폼 델타로 수평 이동+회전을 따라가므로 커브 점프도 안전.
	// (StayBasedInAirHeight=1000 아래에서 유효. 그 위로 높이 뛰면 베이스를 놓고
	//  GODTrain::GetVelocity 기반 속도 impart로 폴백)
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->bStayBasedInAir = true;
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() == true)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			UEnhancedInputLocalPlayerSubsystem* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
			if (EILPS)
			{
				EILPS->AddMappingContext(DefaultMappingContext, 0);
			}
		}
		
	}
}

// ============================================================
// GAS 초기화
// ============================================================

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitializeAbilityActorInfo();
	ServerInitGAS();
}

void ABaseCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	
	InitializeAbilityActorInfo();
}

void ABaseCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 서버/클라 양쪽에서 속성·태그 변경을 듣고 MaxWalkSpeed 를 로컬에 반영.
		InitSpeedBindings();
	}
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaseCharacter::ServerInitGAS()
{
	if (!HasAuthority() || !AbilitySystemComponent || bGASInitialized) return;
	bGASInitialized = true;
	
	ApplyCharacterDataRow(CharacterTag);
}

void ABaseCharacter::ApplyCharacterDataRow(const FGameplayTag& RowTag)
{
	if (!HasAuthority() || !AbilitySystemComponent || !RowTag.IsValid()) return;

	UBaseDataSubsystem* DataSubsystem = GetGameInstance()->GetSubsystem<UBaseDataSubsystem>();
	if (!DataSubsystem) return;

	const FCharacterAttributeRow* Row = DataSubsystem->GetCharacterAttributeRow(RowTag);
	if (!Row) return;

	// 역할 변경 재적용: 유효한 Row 가 있을 때만 이전 부여분을 정리(잘못된 태그로 능력 전부 날아가는 것 방지).
	ClearCharacterDataRow();

	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;
	if (Row->DefaultAttributeGE)
	{
		EffectsToApply.Add(Row->DefaultAttributeGE);
	}
	EffectsToApply.Append(Row->GrantedEffects);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
	{
		if (!EffectClass) continue;

		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// 지속/무한 이펙트만 유효 핸들을 돌려주므로(Instant 는 무효), 추적해 역할 변경 시 제거.
			FActiveGameplayEffectHandle GEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			if (GEHandle.IsValid())
			{
				AppliedEffectHandles.Add(GEHandle);
			}
		}
	}

	// 공통 어빌리티 부여 (예: 총기 발사)
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->CommonAbilities)
	{
		if (AbilityClass)
		{
			GrantedAbilityHandles.Add(
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this)));
		}
	}

	// 직업 전용 어빌리티 부여 (예: E 특수스킬)
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->JobAbilities)
	{
		if (AbilityClass)
		{
			GrantedAbilityHandles.Add(
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this)));
		}
	}
}

void ABaseCharacter::ClearCharacterDataRow()
{
	if (!AbilitySystemComponent) return;

	// 부여했던 지속/무한 이펙트 제거.
	for (const FActiveGameplayEffectHandle& Handle : AppliedEffectHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}
	AppliedEffectHandles.Reset();

	// 부여했던 어빌리티 회수.
	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
		}
	}
	GrantedAbilityHandles.Reset();
}

void ABaseCharacter::SetCharacterTag(const FGameplayTag& NewTag)
{
	CharacterTag = NewTag;

	// GAS 가 이미 초기화된 뒤 역할이 바뀌면, 새 역할의 속성/어빌리티로 다시 적용한다.
	// (아직 초기화 전이면 ServerInitGAS 가 이 태그로 최초 적용한다.)
	if (HasAuthority() && bGASInitialized)
	{
		ApplyCharacterDataRow(CharacterTag);
	}

	// 직업이 바뀌면 짐꾼 무게 면제 여부가 달라지므로 즉시 속도 재계산.
	RecalculateMoveSpeed();
}

// ============================================================
// 이동 속도 (무게 / 디버프 / 직업)
// ============================================================

void ABaseCharacter::InitSpeedBindings()
{
	if (bSpeedBindingsInitialized || !AbilitySystemComponent || !AttributeSet) return;
	bSpeedBindingsInitialized = true;

	// Speed(디버프/날씨/버프 GE 반영분) 또는 Weight 가 바뀌면 MaxWalkSpeed 재계산.
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetSpeedAttribute())
		.AddUObject(this, &ABaseCharacter::HandleSpeedAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetWeightAttribute())
		.AddUObject(this, &ABaseCharacter::HandleSpeedAttributeChanged);

	// 초기 1회 적용.
	RecalculateMoveSpeed();
}

void ABaseCharacter::HandleSpeedAttributeChanged(const FOnAttributeChangeData& /*Data*/)
{
	RecalculateMoveSpeed();
}

void ABaseCharacter::AddWeight(float Delta)
{
	if (!HasAuthority() || !AttributeSet) return;

	// SetWeight 가 base value 를 갱신 → 복제(OnRep) + 속성 변경 델리게이트로 RecalculateMoveSpeed 트리거.
	const float NewWeight = FMath::Max(0.f, AttributeSet->GetWeight() + Delta);
	AttributeSet->SetWeight(NewWeight);
}

void ABaseCharacter::RecalculateMoveSpeed()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC || !AttributeSet) return;

	// Speed CurrentValue 에는 디버프/날씨/버프 GE 의 배수가 이미 GAS 로 합산돼 있다.
	float Result = AttributeSet->GetSpeed();

	// 무게 패널티 — 짐꾼(Porter)은 무게의 영향을 받지 않는다. (연속값 + 조건이라 여기서 처리)
	const bool bIsPorter = (CharacterTag == Character::Crew::Porter.GetTag());
	if (!bIsPorter)
	{
		const float Weight = FMath::Max(0.f, AttributeSet->GetWeight());
		const float WeightMul = FMath::Max(MinWeightSpeedMultiplier, 1.f - Weight * WeightSpeedPenaltyPerUnit);
		Result *= WeightMul;
	}

	CMC->MaxWalkSpeed = FMath::Max(0.f, Result);

	// [임시 디버그] 실제 값 확인용 — 원인 파악 후 이 블록 삭제.
	if (GEngine)
	{
		const TCHAR* NetRole = HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT");
		GEngine->AddOnScreenDebugMessage(
			(int32)GetUniqueID(), 5.f, FColor::Yellow,
			FString::Printf(TEXT("[%s] Speed=%.0f Weight=%.0f Porter=%d -> MaxWalk=%.0f"),
				NetRole, AttributeSet->GetSpeed(), AttributeSet->GetWeight(), bIsPorter ? 1 : 0, CMC->MaxWalkSpeed));
	}
}

// ============================================================
// 리플리케이션
// ============================================================

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseCharacter, bIsDead);
	DOREPLIFETIME(ABaseCharacter, CurrentWeapon);
	DOREPLIFETIME(ABaseCharacter, CurrentHeldItem);
	DOREPLIFETIME(ABaseCharacter, bMeshHidden);
	// 직업 태그는 소유 클라에만 복제(다른 플레이어에게 역할 노출 방지).
	DOREPLIFETIME_CONDITION(ABaseCharacter, CharacterTag, COND_OwnerOnly);
	
	DOREPLIFETIME(ABaseCharacter, bIsCoalEquipped);
}

// ============================================================
// 직업 / 가시성 (투명화·시체 은폐·감별 표식)
// ============================================================

bool ABaseCharacter::IsMafia() const
{
	return CharacterTag == Character::Special::Mafia.GetTag();
}

void ABaseCharacter::SetInvisibleForDuration(float Duration)
{
	if (!HasAuthority()) return;

	bMeshHidden = true;
	ApplyMeshVisibility();

	GetWorldTimerManager().ClearTimer(InvisibleTimerHandle);
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(InvisibleTimerHandle, this, &ABaseCharacter::RevealMesh, Duration, false);
	}
}

void ABaseCharacter::HideCorpseForDuration(float Duration)
{
	if (!HasAuthority()) return;

	bMeshHidden = true;
	ApplyMeshVisibility();

	GetWorldTimerManager().ClearTimer(CorpseHideTimerHandle);
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(CorpseHideTimerHandle, this, &ABaseCharacter::RevealMesh, Duration, false);
	}
}

void ABaseCharacter::RevealMesh()
{
	if (!HasAuthority()) return;

	bMeshHidden = false;
	ApplyMeshVisibility();
}

void ABaseCharacter::OnRep_MeshHidden()
{
	ApplyMeshVisibility();
}

void ABaseCharacter::ApplyMeshVisibility()
{
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetVisibility(!bMeshHidden, true);
	}
	if (CurrentWeapon)
	{
		CurrentWeapon->SetActorHiddenInGame(bMeshHidden);
	}
	if (CurrentHeldItem)
	{
		CurrentHeldItem->SetActorHiddenInGame(bMeshHidden);
	}
	if (WidgetComponent)
	{
		WidgetComponent->SetVisibility(!bMeshHidden, true);
	}
}

void ABaseCharacter::Multicast_PlayNiagaraAtSelf_Implementation(UNiagaraSystem* System)
{
	if (!System || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAttached(
		System, GetMesh(), NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget, true);
}

// ============================================================
// 장착 슬롯 (총/물리 아이템 공용, 한 번에 하나만)
// ============================================================

void ABaseCharacter::ClearEquipSlot()
{
	if (!HasAuthority()) return;

	// 1) 총이 장착돼 있으면 해제 (무기 파괴 + 태그 제거)
	const FGameplayTag GunTag = State::Weapon::EquipGun.GetTag();
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(GunTag))
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
		}
		CurrentWeapon = nullptr;

		AbilitySystemComponent->RemoveLooseGameplayTag(GunTag);
		AbilitySystemComponent->RemoveReplicatedLooseGameplayTag(GunTag);
	}

	// 2) 물리 아이템을 들고 있으면 떨군다.
	//    Server_Drop 이 자체적으로 장착 태그/CurrentHeldItem 까지 정리한다.
	if (IsValid(CurrentHeldItem))
	{
		CurrentHeldItem->Server_Drop();
	}
	CurrentHeldItem = nullptr;
}

// ============================================================
// 사망 처리
// ============================================================

void ABaseCharacter::Die(AActor* Killer)
{
	if (!HasAuthority() || bIsDead) return;

	bIsDead = true;

	// HandlePlayerDeath를 거치지 않는 직접 호출에서도 두 사망 상태가 일치하도록 동기화.
	if (AGODPlayerState* PS = GetPlayerState<AGODPlayerState>())
	{
		PS->bIsAlive = false;
	}

	OnCharacterDied.Broadcast(this, Killer);

	// 킬러에게 처치 이벤트 전송 → 마피아 시체 은폐 등 패시브 어빌리티 트리거(피해자 = 이 캐릭터).
	if (ABaseCharacter* KillerChar = Cast<ABaseCharacter>(Killer))
	{
		if (UAbilitySystemComponent* KillerASC = KillerChar->GetAbilitySystemComponent())
		{
			FGameplayEventData Payload;
			Payload.EventTag = Event::Kill.GetTag();
			Payload.Instigator = KillerChar;
			Payload.Target = this;
			KillerASC->HandleGameplayEvent(Event::Kill.GetTag(), &Payload);
		}
	}

	Multicast_HandleDeath();
}

void ABaseCharacter::Multicast_HandleDeath_Implementation()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (GetMesh())
	{
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	}
	
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
}

void ABaseCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		Multicast_HandleDeath_Implementation();
	}
}

// ============================================================
// IInteractable
// ============================================================

void ABaseCharacter::Interact_Implementation(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor) return;

	FGameplayEventData EventData;
	EventData.OptionalObject = this;

	const FGameplayTag TriggerTag = bIsDead
		? FGameplayTag::RequestGameplayTag(TEXT("Ability.Trigger.StealAmmo"))
		: FGameplayTag::RequestGameplayTag(TEXT("Ability.Trigger.SearchTarget"));

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Interactor, TriggerTag, EventData);
}

FText ABaseCharacter::GetInteractPrompt_Implementation() const
{
	return bIsDead
		? FText::FromString(TEXT("탄약 빼앗기"))
		: FText::FromString(TEXT("수색"));
}

void ABaseCharacter::SetCoalEquipped(bool bEquip)
{
	if (!HasAuthority()) return;

	if (bIsCoalEquipped == bEquip)
		return;

	bIsCoalEquipped = bEquip;
	
	
	UpdateCoalVisual();
}

void ABaseCharacter::OnRep_CoalEquipped()
{
	UpdateCoalVisual();
}


void ABaseCharacter::UpdateCoalVisual()
{
	

	if (bIsCoalEquipped)
	{
		
		SpawnCoalHands();
	}
	else
	{
		
		DestroyCoalHands();
	}
}
void ABaseCharacter::SpawnCoalHands()
{
	

	if (LeftCoalActor || RightCoalActor)
	{
		
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	if (!CoalHandClass)
	{
		return;
	}
	

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;

	LeftCoalActor = GetWorld()->SpawnActor<AACoalHandVisualActor>(
		CoalHandClass,
		MeshComp->GetComponentTransform(),
		Params
	);
	

	if (LeftCoalActor)
	{
		LeftCoalActor->AttachToComponent(
			MeshComp,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("LeftHand_end")
		);
		LeftCoalActor->SetActorScale3D(FVector(0.1f));
	}

	RightCoalActor = GetWorld()->SpawnActor<AACoalHandVisualActor>(
		CoalHandClass,
		MeshComp->GetComponentTransform(),
		Params
	);
	

	if (RightCoalActor)
	{
		RightCoalActor->AttachToComponent(
			MeshComp,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			TEXT("RightHand_end")
		);
		RightCoalActor->SetActorScale3D(FVector(0.1f));
	}
}
void ABaseCharacter::DestroyCoalHands()
{
	if (LeftCoalActor)
	{
		LeftCoalActor->Destroy();
		LeftCoalActor = nullptr;
	}

	if (RightCoalActor)
	{
		RightCoalActor->Destroy();
		RightCoalActor = nullptr;
	}
}

// ============================================================
// Input
// ============================================================
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Look);
	}
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABaseCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
