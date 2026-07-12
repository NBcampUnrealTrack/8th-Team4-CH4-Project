// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BaseCharacter.h"
#include "Player/GODPlayerState.h"
#include "Player/VoiceChannelSubsystem.h"
#include "Player/BasePlayerController.h"
#include "Player/Component/BaseAbilitySystemComponent.h"
#include "Player/Component/BaseAttributeSet.h"
#include "Player/Weapon/BaseWeapon.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/ItemBase.h"
#include "InteractiveProp/DoorBase.h"
#include "InteractiveProp/PressureValve.h"
#include "InteractiveProp/GearSlot.h"
#include "Game/BaseGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Game/BaseDataSubsystem.h"
#include "Game/PlayerGameInstance.h"
#include "Game/GODGameUserSettings.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Game/GODGameState.h"
#include "Game/GODGameMode.h"
#include "InteractiveProp/GODTrain.h"
#include "UI/HUD/GODHUD.h"
#include "EngineUtils.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/Engine.h" // [임시] 디버그 화면 출력용
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputActionValue.h"
#include "Component/CustomMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"
ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UBaseAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));

	InteractComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractComponent"));

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);

	// 순찰자 랜턴 — 기본 꺼진 상태.
	// 동물 스킨 5종 스켈레톤에는 hand_l 소켓이 없어 메시 원점(발밑, 옆방향)에 붙어버리므로
	// 캡슐(루트)에 부착해 스켈레톤과 무관하게 항상 캐릭터 정면(+X)·가슴 높이를 비춘다.
	LanternLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(RootComponent);
	LanternLight->SetRelativeLocation(FVector(40.f, 0.f, 50.f));
	LanternLight->SetRelativeRotation(FRotator::ZeroRotator);
	LanternLight->SetVisibility(false);
	LanternLight->Intensity     = 3000.f;
	LanternLight->OuterConeAngle = 35.f;

	GetCapsuleComponent()->InitCapsuleSize(30.f, 48.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CustomMovementComponent = Cast<UCustomMovementComponent>(GetCharacterMovement());

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

		// 캐릭터가 서로/물리 오브젝트를 힘으로 밀어내는 상호작용을 끈다(밀침 완화).
		CMC->bEnablePhysicsInteraction = false;
		// RVO 회피(서로 미끄러지듯 비켜가는 밀림)도 끈다.
		CMC->bUseRVOAvoidance = false;

		// 문/기차 같은 키네마틱 지오메트리에 캡슐이 파묻혔을 때 한 프레임에
		// 밀어내는 최대 거리. 기본값 500cm는 얇은 기차 벽을 그대로 관통해
		// 차량 밖으로 사출되는 원인이 되므로 벽 두께보다 작게 제한한다.
		CMC->MaxDepenetrationWithGeometry = 30.f;
		CMC->MaxDepenetrationWithGeometryAsProxy = 30.f;
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 아웃로 가짜 시체 복귀에 필요한 초기 메시 트랜스폼 저장.
	if (GetMesh())
	{
		DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
		DefaultMeshRelativeRotation = GetMesh()->GetRelativeRotation();

		// 툰 포스트프로세스(PP_ToonShader)용 캐릭터 마스크.
		// CustomStencil=1 인 픽셀만 셀 셰이딩/외곽선을 받고, 배경은 색보정만 받는다.
		GetMesh()->SetRenderCustomDepth(true);
		GetMesh()->SetCustomDepthStencilValue(1);
	}

	if (IsLocallyControlled() == true)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			UEnhancedInputLocalPlayerSubsystem* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
			if (EILPS)
			{
				EILPS->AddMappingContext(DefaultMappingContext, 0);

				// 설정창 조작 탭 리바인딩 목록에 이 IMC 의 매핑이 뜨도록 User Settings 에 등록.
				// (Project Settings → Enhanced Input → Enable User Settings 필요)
				if (UEnhancedInputUserSettings* InputSettings = EILPS->GetUserSettings())
				{
					InputSettings->RegisterInputMappingContext(DefaultMappingContext);
				}
			}
		}

		SendSkinSelectionToServer();
	}

	CacheFallReferenceActors();

	// 열차 밖으로 떨어졌을 때 감시 (서버 전용, KillZ 미설정 맵 대비).
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			FallRescueTimer, this, &ABaseCharacter::CheckFallRescueOrDeath, 1.f, /*bLoop=*/true);
	}
}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateRoleTimers();
	Super::EndPlay(EndPlayReason);
}

// ============================================================
// GAS 초기화
// ============================================================

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitializeAbilityActorInfo();
	ServerInitGAS();

	// 리슨 서버(호스트) 본인 캐릭터: BeginPlay 시점엔 아직 빙의 전이라 IsLocallyControlled 가 false 고,
	// OnRep_Controller 는 서버에서 호출되지 않으므로 여기서 스킨을 적용한다.
	if (IsLocallyControlled())
	{
		SendSkinSelectionToServer();
	}
}

void ABaseCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	InitializeAbilityActorInfo();

	// 클라에서 BeginPlay 시점엔 컨트롤러가 아직 복제 전일 수 있어 여기서도 시도.
	if (IsLocallyControlled())
	{
		SendSkinSelectionToServer();
	}
}

// ============================================================
// 스킨 (동물 5종)
// ============================================================

void ABaseCharacter::SendSkinSelectionToServer()
{
	if (bSkinSelectionSent) return;

	if (const UPlayerGameInstance* GI = GetGameInstance<UPlayerGameInstance>())
	{
		bSkinSelectionSent = true;
		Server_SetSkin(GI->SelectedSkinIndex);
	}
}

void ABaseCharacter::Server_SetSkin_Implementation(int32 InSkinIndex)
{
	if (!SkinOptions.IsValidIndex(InSkinIndex)) return;

	SkinIndex = InSkinIndex;
	ApplySkin(); // 리슨 서버(호스트) 화면 반영 — 클라는 OnRep 에서 반영
}

void ABaseCharacter::OnRep_SkinIndex()
{
	ApplySkin();
}

void ABaseCharacter::ApplySkin()
{
	if (!SkinOptions.IsValidIndex(SkinIndex)) return;

	const FCharacterSkinData& Skin = SkinOptions[SkinIndex];
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !Skin.Mesh) return;

	MeshComp->SetSkeletalMesh(Skin.Mesh);
	MeshComp->SetRelativeScale3D(Skin.MeshScale);
	if (Skin.AnimClass)
	{
		MeshComp->SetAnimInstanceClass(Skin.AnimClass);
	}
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
	RefreshRoleLooseTag();
}

void ABaseCharacter::RefreshRoleLooseTag()
{
	if (!AbilitySystemComponent || AppliedRoleLooseTag == CharacterTag) return;

	if (AppliedRoleLooseTag.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(AppliedRoleLooseTag);
	}
	if (CharacterTag.IsValid())
	{
		AbilitySystemComponent->AddLooseGameplayTag(CharacterTag);
	}
	AppliedRoleLooseTag = CharacterTag;
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

	// 게임 시작 전(대기/카운트다운)에는 어빌리티를 부여하지 않는다 — 속성/이펙트 GE 만 적용.
	// 로비에서 총 장착/발사, 역할 능력이 쓰이는 것을 막는다.
	// 어빌리티는 StartGame() → AssignRoles() → SetCharacterTag() 재적용 시(Playing 페이즈) 부여된다.
	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;
	if (!GS || GS->CurrentPhase != EGamePhase::Playing)
	{
		return;
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
	// 기존 역할 타이머를 먼저 정리한다.
	if (HasAuthority()) DeactivateRoleTimers();

	CharacterTag = NewTag;

	// 역할별 기본 수치 반영 (정비공은 수리 속도 2배).
	RepairSpeedMultiplier = (NewTag == Character::Crew::Mechanic.GetTag()) ? 2.0f : 1.0f;

	// GAS 가 이미 초기화된 뒤 역할이 바뀌면, 새 역할의 속성/어빌리티로 다시 적용한다.
	// (아직 초기화 전이면 ServerInitGAS 가 이 태그로 최초 적용한다.)
	if (HasAuthority() && bGASInitialized)
	{
		ApplyCharacterDataRow(CharacterTag);
	}

	// 직업이 바뀌면 짐꾼 무게 면제 여부가 달라지므로 즉시 속도 재계산.
	RecalculateMoveSpeed();

	// 새 역할에 맞는 패시브 타이머 시작.
	if (HasAuthority()) ActivateRoleTimers();

	// 역할 능력의 ActivationRequiredTags 검사용 루즈 태그 동기화 (서버 측).
	RefreshRoleLooseTag();

	// 리슨 서버(호스트) 본인 HUD 갱신용 — 원격 클라는 OnRep_CharacterTag 에서 발화.
	OnCharacterTagChanged.Broadcast(CharacterTag);
}

void ABaseCharacter::OnRep_CharacterTag()
{
	// 짐꾼 무게 면제 등 태그 의존 로직을 클라에서도 즉시 반영.
	RecalculateMoveSpeed();

	// 소유 클라 측 루즈 태그 동기화 — 클라에서 TryActivateAbility 의 사전 검사
	// (ActivationRequiredTags)가 통과해야 서버로 발동 요청이 간다.
	RefreshRoleLooseTag();

	OnCharacterTagChanged.Broadcast(CharacterTag);
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
	DOREPLIFETIME(ABaseCharacter, SkinIndex);
	// 직업 태그는 소유 클라에만 복제(다른 플레이어에게 역할 노출 방지).
	DOREPLIFETIME_CONDITION(ABaseCharacter, CharacterTag, COND_OwnerOnly);
	
	DOREPLIFETIME(ABaseCharacter, CurrentCoal);

	// 역할별 상태
	DOREPLIFETIME(ABaseCharacter, bIsInVent);
	DOREPLIFETIME(ABaseCharacter, bIsInvisible);
	DOREPLIFETIME(ABaseCharacter, bIsFakeDead);
	// 랜턴은 본인 화면에만 보이므로 소유 클라에만 복제 (다른 클라가 값을 읽어 역할 유추하는 것도 차단).
	DOREPLIFETIME_CONDITION(ABaseCharacter, bLanternOn, COND_OwnerOnly);
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
	// 마피아 투명화: 자신을 제외한 모든 클라이언트에서 숨김.
	if (bIsInvisible && !IsLocallyControlled())
	{
		if (GetMesh())
		{
			GetMesh()->SetVisibility(false);
		}
		if (CurrentWeapon)
		{
			CurrentWeapon->SetActorHiddenInGame(true);
		}
		if (CurrentHeldItem)
		{
			CurrentHeldItem->SetActorHiddenInGame(true);
		}
		if (WidgetComponent)
		{
			WidgetComponent->SetVisibility(false, true);
		}
		if (LanternLight)
		{
			LanternLight->SetVisibility(false);
		}
		return;
	}

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
	// 위 SetVisibility(..., true) 전파가 자식 컴포넌트를 전부 켜버리므로
	// 랜턴은 토글 상태(bLanternOn)로 마지막에 복원한다.
	// 빛으로 순찰자 역할이 유추되면 안 되므로 본인 화면에서만 보인다.
	if (LanternLight)
	{
		LanternLight->SetVisibility(bLanternOn && !bMeshHidden && IsLocallyControlled());
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
// 사운드 — 캐릭터 사운드 DT
// ============================================================

void ABaseCharacter::PlayCharacterSoundLocal(FName RowName)
{
	UGameSoundStatics::PlaySoundAtLocationFromTable(this, CharacterSoundTable, RowName, GetActorLocation());
}

void ABaseCharacter::Multicast_PlayCharacterSound_Implementation(FName RowName)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	PlayCharacterSoundLocal(RowName);
}

void ABaseCharacter::Client_PlayCharacterSound_Implementation(FName RowName)
{
	PlayCharacterSoundLocal(RowName);
}

void ABaseCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if (GetNetMode() != NM_DedicatedServer)
	{
		PlayCharacterSoundLocal(SoundRows::FootstepJump);
	}
}

void ABaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (GetNetMode() != NM_DedicatedServer)
	{
		PlayCharacterSoundLocal(SoundRows::FootstepLand);
	}
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

void ABaseCharacter::DropHeldItem()
{
	if (bInputLockedByMinigame || bIsDead) return;
	Server_DropHeldItem();
}

void ABaseCharacter::Server_DropHeldItem_Implementation()
{
	if (bIsDead) return;

	if (IsValid(CurrentHeldItem))
	{
		CurrentHeldItem->Server_Drop(); // 서버 로컬 호출 → 즉시 실행

		// 버리기 사운드는 플레이어가 직접 버리는 이 경로에서만 낸다
		// (화로 투입/기어 조립 내부의 Server_Drop 에서는 각자의 사운드가 따로 있다).
		Multicast_PlayCharacterSound(SoundRows::ItemDrop);
	}
}

// ============================================================
// 사망 처리
// ============================================================

float ABaseCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// 화부(Stoker)는 열 피해에 완전 면역.
	if (CharacterTag == Character::Crew::Stoker.GetTag())
	{
		if (DamageCauser && DamageCauser->ActorHasTag(TEXT("DamageSource.Heat")))
		{
			return 0.f;
		}
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

// ============================================================
// 밀치기 (총 미장착 좌클릭)
// ============================================================
UAnimMontage* ABaseCharacter::ResolvePushMontage(bool bStumble) const
{
	// 동물 스킨마다 스켈레톤이 다르므로 몽타주도 스킨별로 지정한다.
	if (SkinOptions.IsValidIndex(SkinIndex))
	{
		const FCharacterSkinData& Skin = SkinOptions[SkinIndex];
		if (UAnimMontage* SkinMontage = bStumble ? Skin.StumbleMontage : Skin.PushMontage)
		{
			return SkinMontage;
		}
	}

	return bStumble ? StumbleMontage : PushMontage;
}

void ABaseCharacter::PlayMontageLocal(UAnimMontage* Montage)
{
	if (!Montage || GetNetMode() == NM_DedicatedServer) return;

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	// 스킨마다 스켈레톤이 다르다. 맞지 않는 몽타주를 재생하면 포즈가 깨지므로 거른다.
	const USkeletalMesh* CurrentMesh = MeshComp->GetSkeletalMeshAsset();
	if (CurrentMesh && Montage->GetSkeleton() != CurrentMesh->GetSkeleton()) return;

	AnimInstance->Montage_Play(Montage);
}

void ABaseCharacter::Multicast_PlayPushMontage_Implementation()
{
	PlayMontageLocal(ResolvePushMontage(/*bStumble=*/false));
}

void ABaseCharacter::Multicast_PlayStumbleMontage_Implementation()
{
	PlayMontageLocal(ResolvePushMontage(/*bStumble=*/true));
}

void ABaseCharacter::ReceivePush(ABaseCharacter* Pusher, const FVector& LaunchVelocity, float StumbleDuration)
{
	if (!HasAuthority() || bIsDead) return;

	// 낙사 킬 크레딧용. 이 창(PushKillCreditWindow) 안에 떨어져 죽으면 밀친 사람이 킬러가 된다.
	LastPusher = Pusher;
	LastPushedTime = GetWorld()->GetTimeSeconds();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(State::Stumble.GetTag());
		GetWorldTimerManager().SetTimer(
			StumbleTimer, this, &ABaseCharacter::EndStumble, StumbleDuration, false);
	}

	Multicast_PlayStumbleMontage();

	// 넉백은 서버 시뮬과 소유 클라 양쪽에 적용해야 위치 보정으로 되돌아가지 않는다.
	if (IsLocallyControlled())
	{
		ApplyLocalStumble(LaunchVelocity, StumbleDuration);
	}
	else
	{
		LaunchCharacter(LaunchVelocity, true, true);
		Client_LaunchFromPush(LaunchVelocity, StumbleDuration);
	}
}

void ABaseCharacter::Client_LaunchFromPush_Implementation(FVector LaunchVelocity, float StumbleDuration)
{
	ApplyLocalStumble(LaunchVelocity, StumbleDuration);
}

void ABaseCharacter::ApplyLocalStumble(const FVector& LaunchVelocity, float StumbleDuration)
{
	LaunchCharacter(LaunchVelocity, true, true);

	// 비틀거리는 동안 이동 입력 잠금. 잠그지 않으면 공중조작으로 낙하를 그대로 되돌릴 수 있다.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// SetIgnoreMoveInput 은 누적 카운터다. 연속으로 밀리면 타이머만 갱신하고
		// 잠금은 한 번만 걸어야 해제 시 카운터가 0으로 돌아온다.
		if (!GetWorldTimerManager().IsTimerActive(StumbleInputLockTimer))
		{
			PC->SetIgnoreMoveInput(true);
		}

		// 캐릭터가 아니라 컨트롤러를 캡처한다. 밀려서 죽어 폰이 파괴돼도
		// 관전 중인 컨트롤러의 입력 잠금은 반드시 풀어야 한다.
		GetWorldTimerManager().SetTimer(
			StumbleInputLockTimer,
			[WeakPC = TWeakObjectPtr<APlayerController>(PC)]()
			{
				if (WeakPC.IsValid())
				{
					WeakPC->SetIgnoreMoveInput(false);
				}
			},
			StumbleDuration, false);
	}
}

void ABaseCharacter::EndStumble()
{
	if (!HasAuthority() || !AbilitySystemComponent) return;
	AbilitySystemComponent->RemoveLooseGameplayTag(State::Stumble.GetTag());
}

AGODPlayerState* ABaseCharacter::GetFallKillCredit() const
{
	if (!LastPusher.IsValid()) return nullptr;
	if (GetWorld()->GetTimeSeconds() - LastPushedTime > PushKillCreditWindow) return nullptr;

	return LastPusher->GetPlayerState<AGODPlayerState>();
}

void ABaseCharacter::Die(AActor* Killer)
{
	if (!HasAuthority() || bIsDead) return;

	bIsDead = true;

	// 랜턴이 켜진 채 죽으면 래그돌은 날아가고 캡슐 위치에 불빛만 남으므로 꺼준다.
	if (bLanternOn)
	{
		bLanternOn = false;
		OnRep_LanternOn();
	}

	// 들고 있던 아이템은 시체 손에 남으면 다시 주울 수 없으므로 그 자리에 떨어뜨린다.
	// (석탄 비주얼은 떨어뜨릴 실물이 없으므로 제거만 한다)
	if (IsValid(CurrentHeldItem))
	{
		CurrentHeldItem->Server_Drop();
	}
	SetCoalEquipped(false);

	// HandlePlayerDeath를 거치지 않는 직접 호출에서도 두 사망 상태가 일치하도록 동기화.
	// (SetIsAlive가 보이스 채널 격리도 함께 갱신한다)
	if (AGODPlayerState* PS = GetPlayerState<AGODPlayerState>())
	{
		PS->SetIsAlive(false);
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

	// 어떤 경로로 죽든(총격 등 Die 직접 호출 포함) 즉시 관전 모드로 전환한다.
	// 서버에서 호출 → Client RPC로 해당 클라의 카메라를 살아있는 플레이어로 이동.
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		PC->Client_StartSpectating();
	}
}

void ABaseCharacter::ResetDeathState()
{
	if (!HasAuthority() || !bIsDead)
	{
		return;
	}

	bIsDead = false;

	// 리슨 서버(호스트)는 OnRep이 안 오므로 직접 호출 → 보이스 정책 재적용.
	// (클라이언트는 bIsDead 복제 도착 시 OnRep_IsDead 가 각자 갱신)
	OnRep_IsDead();
}

void ABaseCharacter::Multicast_HandleDeath_Implementation()
{
	// 사망음 (전 클라, 이 위치). 무법자 죽은 척도 같은 행을 재생해 구분되지 않게 한다.
	if (GetNetMode() != NM_DedicatedServer)
	{
		PlayCharacterSoundLocal(SoundRows::CharacterDie);
	}

	// 사망 시점에 밟고 있던 발판(열차 등)과 공중 여부를 먼저 기억한다. (움직임 비활성화 전에 조회)
	UPrimitiveComponent* DeathBase = nullptr;
	bool bDiedInAir = false;
	if (GetCharacterMovement())
	{
		DeathBase = GetCharacterMovement()->GetMovementBase();
		bDiedInAir = GetCharacterMovement()->IsFalling();
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

	AttachRagdollToBase(DeathBase, bDiedInAir);
}

void ABaseCharacter::AttachRagdollToBase(UPrimitiveComponent* Base, bool bInAir)
{
	// 래그돌이 열차와 함께 이동하도록 발판(열차 메시)에 부착한다.
	// 열차는 순간이동 방식이라, 부착 안 하고 래그돌이 시뮬 중이면 열차 콜리전에
	// 튕겨 날아간다. GetMovementBase가 비면(클라 타이밍 등) 열차를 직접 찾아 폴백한다.
	if (!Base && IsValid(CachedTrain))
	{
		Base = CachedTrain->TrainMesh;
	}

	if (!Base)
	{
		return;
	}

	if (bInAir)
	{
		// 공중: 즉시 kinematic 고정하면 몸이 공중에 뜬 채 남으므로,
		// 래그돌이 바닥에 떨어져 멈춘 뒤에 부착한다.
		WaitForCorpseToLand(Base);
	}
	else if (CorpseSettleTime <= 0.f)
	{
		// 즉시 부착(현재 포즈로 고정) → 열차 위에서 바로 함께 이동.
		AttachCorpseToBase(Base);
	}
	else
	{
		// 짧게 래그돌 후 부착 (빠른 열차에선 그 사이 뒤로 밀릴 수 있음).
		TWeakObjectPtr<ABaseCharacter> WeakThis(this);
		TWeakObjectPtr<UPrimitiveComponent> WeakBase(Base);
		FTimerDelegate Del = FTimerDelegate::CreateLambda([WeakThis, WeakBase]()
		{
			if (WeakThis.IsValid() && WeakBase.IsValid())
			{
				WeakThis->AttachCorpseToBase(WeakBase.Get());
			}
		});
		GetWorldTimerManager().SetTimer(CorpseAttachTimer, Del, CorpseSettleTime, /*bLoop=*/false);
	}
}

void ABaseCharacter::AttachCorpseToBase(UPrimitiveComponent* Base)
{
	AActor* BaseActor = Base ? Base->GetOwner() : nullptr;
	if (!BaseActor)
	{
		return;
	}

	// 래그돌 바디를 kinematic으로 전환(현재 포즈 유지, 물리 시뮬만 정지)한다.
	// kinematic 바디는 컴포넌트를 따라가므로, 이후 열차에 부착되면 함께 이동한다.
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetAllBodiesSimulatePhysics(false);
	}

	// 액터 전체(캡슐 루트 + 메시)를 발판(열차)에 부착 → 시체 위치/상호작용까지 열차 따라 이동.
	AttachToActor(BaseActor, FAttachmentTransformRules::KeepWorldTransform);
}

void ABaseCharacter::WaitForCorpseToLand(UPrimitiveComponent* Base)
{
	TWeakObjectPtr<ABaseCharacter> WeakThis(this);
	TWeakObjectPtr<UPrimitiveComponent> WeakBase(Base);
	const float StartTime = GetWorld()->GetTimeSeconds();

	FTimerDelegate Del = FTimerDelegate::CreateLambda([WeakThis, WeakBase, StartTime]()
	{
		if (!WeakThis.IsValid()) return;
		ABaseCharacter* Self = WeakThis.Get();

		const float Elapsed = Self->GetWorld()->GetTimeSeconds() - StartTime;

		// 래그돌 속도가 거의 0이면 착지로 판정. (첫 0.3초는 낙하 시작 직후라 제외)
		// 제한 시간을 넘기면 그 자리에서라도 부착해 열차 콜리전에 튕기는 것을 막는다.
		USkeletalMeshComponent* M = Self->GetMesh();
		const bool bSettled = (Elapsed > 0.3f) && M && M->GetComponentVelocity().Size() < 50.f;

		if (bSettled || Elapsed > Self->CorpseFallAttachTimeout)
		{
			Self->GetWorldTimerManager().ClearTimer(Self->CorpseAttachTimer);
			if (WeakBase.IsValid())
			{
				Self->AttachCorpseToBase(WeakBase.Get());
			}
		}
	});

	GetWorldTimerManager().SetTimer(CorpseAttachTimer, Del, 0.15f, /*bLoop=*/true);
}

void ABaseCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		Multicast_HandleDeath_Implementation();
	}

	// 래그돌 사망 복제 도착 → 발화 중인 보이스 격리 정책 재적용.
	// (bIsAlive 복제와 별개 경로로 죽는 케이스 대비 — 보이스 판정은 둘 중 하나만 죽어도 사망 취급)
	if (UVoiceChannelSubsystem* Voice = UVoiceChannelSubsystem::Get(GetWorld()))
	{
		Voice->RefreshVoicePolicies();
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

	// 이미 원하는 상태면 무시 (무기 장착 토글과 동일한 아이덤포턴트 처리).
	if ((CurrentCoal != nullptr) == bEquip)
		return;

	if (bEquip)
	{
		// ── 장착: 무기(GA_EquipGun)와 동일하게 단일 액터 스폰 → 소켓 부착 → 포인터 보관 ──
		if (!CoalHandClass) return;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AACoalHandVisualActor* Coal = GetWorld()->SpawnActor<AACoalHandVisualActor>(
			CoalHandClass, GetActorTransform(), SpawnParams);
		if (Coal)
		{
			// 서버 부착 + 소켓/소유자 복제 → 클라도 OnRep 에서 동일하게 부착 (무기와 동일).
			Coal->AttachToCharacter(this, CoalAttachSocketName);
			CurrentCoal = Coal;
		}
	}
	else
	{
		// ── 해제: 무기와 동일하게 액터 파괴 + 포인터 정리 ──
		if (CurrentCoal)
		{
			CurrentCoal->Destroy();
			CurrentCoal = nullptr;
		}
	}
}

// ============================================================
// 역할 타이머 관리
// ============================================================

void ABaseCharacter::ActivateRoleTimers()
{
	if (!CharacterTag.IsValid()) return;

	if (CharacterTag == Character::Special::Sheriff.GetTag())
	{
		GetWorldTimerManager().SetTimer(
			BodyDetectionTimer, this, &ABaseCharacter::CheckForHiddenBodies,
			BodyDetectionInterval, /*bLoop=*/true);
	}
	else if (CharacterTag == Character::Crew::Watchman.GetTag())
	{
		GetWorldTimerManager().SetTimer(
			FootprintRecordTimer, this, &ABaseCharacter::RecordFootprintPositions,
			RecordInterval, /*bLoop=*/true);
	}
}

void ABaseCharacter::DeactivateRoleTimers()
{
	GetWorldTimerManager().ClearTimer(BodyDetectionTimer);
	GetWorldTimerManager().ClearTimer(FootprintRecordTimer);
}

// ============================================================
// 역할별 능력 — Mafia
// ============================================================

void ABaseCharacter::EnterVent(AActor* VentActor)
{
	if (!HasAuthority() || bIsInVent) return;
	bIsInVent = true;
	ApplyVentMovement(true);
}

void ABaseCharacter::ExitVent()
{
	if (!HasAuthority() || !bIsInVent) return;
	bIsInVent = false;
	ApplyVentMovement(false);
}

void ABaseCharacter::OnRep_IsInVent()
{
	ApplyVentMovement(bIsInVent);
}

void ABaseCharacter::ApplyVentMovement(bool bEntering)
{
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		if (bEntering)
			CMC->MaxWalkSpeed = 150.f;
		else
			RecalculateMoveSpeed();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->SetCapsuleHalfHeight(bEntering ? 24.f : 96.0f);
}

void ABaseCharacter::SetInvisible(bool bNewInvisible)
{
	if (!HasAuthority()) return;
	bIsInvisible = bNewInvisible;
	ApplyMeshVisibility();
}

void ABaseCharacter::OnRep_IsInvisible()
{
	ApplyMeshVisibility();
}

void ABaseCharacter::UseMasterKey(AActor* DoorActor)
{
	if (!HasAuthority() || !IsValid(DoorActor)) return;

	if (ADoorBase* Door = Cast<ADoorBase>(DoorActor))
		Door->SetLocked(true);
}

void ABaseCharacter::UseWireCutter(AActor* GearActor)
{
	if (!HasAuthority() || !IsValid(GearActor)) return;

	if (AGearSlot* Slot = Cast<AGearSlot>(GearActor))
		Slot->BreakGear();
}

void ABaseCharacter::Multicast_HideBody_Implementation(ABaseCharacter* DeadCharacter)
{
	if (!IsValid(DeadCharacter)) return;
	if (DeadCharacter->GetMesh()) DeadCharacter->GetMesh()->SetVisibility(false);
	if (HasAuthority()) DeadCharacter->Tags.AddUnique(TEXT("Body.Hidden"));
}

void ABaseCharacter::Multicast_ShowBody_Implementation(ABaseCharacter* DeadCharacter)
{
	if (!IsValid(DeadCharacter)) return;
	if (DeadCharacter->GetMesh()) DeadCharacter->GetMesh()->SetVisibility(true);
	if (HasAuthority()) DeadCharacter->Tags.Remove(TEXT("Body.Hidden"));
}

// ============================================================
// 역할별 능력 — Sheriff
// ============================================================

void ABaseCharacter::CheckForHiddenBodies()
{
	TArray<AActor*> Overlapping;
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		BodyDetectionRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{
			UEngineTypes::ConvertToObjectType(ECC_Pawn),
			UEngineTypes::ConvertToObjectType(ECC_PhysicsBody)
		},
		ABaseCharacter::StaticClass(),
		TArray<AActor*>{ this },
		Overlapping
	);

	for (AActor* Actor : Overlapping)
	{
		ABaseCharacter* DeadChar = Cast<ABaseCharacter>(Actor);
		if (DeadChar && DeadChar->IsDead() && DeadChar->ActorHasTag(TEXT("Body.Hidden")))
			OnHiddenBodyDetected(DeadChar);
	}
}

// ============================================================
// 낙하 처리 (로비=복귀 / 진행 중=사망)
// ============================================================

void ABaseCharacter::CacheFallReferenceActors()
{
	for (TActorIterator<AGODTrain> It(GetWorld()); It; ++It)
	{
		CachedTrain = *It;
		break;
	}

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		CachedPlayerStart = *It;
		break;
	}
}

void ABaseCharacter::CheckFallRescueOrDeath()
{
	if (!HasAuthority() || bIsDead) return;

	const AGODGameState* GS = GetWorld()->GetGameState<AGODGameState>();
	if (!GS) return;

	if (GS->CurrentPhase == EGamePhase::WaitingForPlayers ||
	    GS->CurrentPhase == EGamePhase::Countdown)
	{
		// 로비 동안 열차는 스타트 지점에 정차해 있으므로 PlayerStart 높이를 기준으로 낙하를 판정.
		if (IsValid(CachedPlayerStart) &&
			GetActorLocation().Z < CachedPlayerStart->GetActorLocation().Z - LobbyFallRescueDepth)
		{
			RescueToStart();
		}
		return;
	}

	if (GS->CurrentPhase != EGamePhase::Playing) return;

	// 주행 중에는 지형이 열차보다 높아 KillZ 에 닿지 않는 구간이 있으므로
	// FellOutOfWorld 를 기다리지 않고 열차와의 고도차로 직접 판정한다.
	float ReferenceZ = 0.f;
	if (!GetFallReferenceZ(ReferenceZ)) return;

	if (GetActorLocation().Z < ReferenceZ - FallDeathDepth)
	{
		// 승리조건 체크를 타려면 반드시 GameMode 경유 (Die() 직접 호출 금지).
		// 최근에 밀린 거라면 밀친 사람이 킬러로 기록된다.
		if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
		{
			GM->HandlePlayerDeath(GetFallKillCredit(), GetPlayerState<AGODPlayerState>());
		}
	}
}

bool ABaseCharacter::GetFallReferenceZ(float& OutZ) const
{
	if (IsValid(CachedTrain))
	{
		OutZ = CachedTrain->GetActorLocation().Z;
		return true;
	}

	if (IsValid(CachedPlayerStart))
	{
		OutZ = CachedPlayerStart->GetActorLocation().Z;
		return true;
	}

	return false;
}

bool ABaseCharacter::RescueToStart()
{
	APlayerStart* Start = CachedPlayerStart;
	if (!IsValid(Start)) return false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately(); // 낙하 속도 제거
	}

	if (!TeleportTo(Start->GetActorLocation(), Start->GetActorRotation()))
	{
		// 다른 플레이어와 겹치는 등으로 실패하면 강제 이동.
		SetActorLocation(Start->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	return true;
}

void ABaseCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	const AGODGameState* GS = GetWorld() ? GetWorld()->GetGameState<AGODGameState>() : nullptr;

	if (HasAuthority() && GS && !bIsDead)
	{
		// 로비 페이즈면 파괴하지 않고 열차(스타트 지점)로 복귀.
		const bool bLobbyPhase = GS->CurrentPhase == EGamePhase::WaitingForPlayers ||
		                         GS->CurrentPhase == EGamePhase::Countdown;
		if (bLobbyPhase && RescueToStart())
		{
			return;
		}

		// 진행 중이면 폰을 그냥 파괴하지 않고 사망 경로를 태운다. (관전 전환 + 승리조건 체크)
		if (GS->CurrentPhase == EGamePhase::Playing)
		{
			if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
			{
				GM->HandlePlayerDeath(GetFallKillCredit(), GetPlayerState<AGODPlayerState>());
				return;
			}
		}
	}

	Super::FellOutOfWorld(dmgType);
}

void ABaseCharacter::UnlockDoor(AActor* DoorActor)
{
	if (!HasAuthority() || !IsValid(DoorActor)) return;

	if (ADoorBase* Door = Cast<ADoorBase>(DoorActor))
		Door->SetLocked(false);
}

// ============================================================
// 역할별 능력 — Outlaw
// ============================================================

void ABaseCharacter::StartFakeDeath()
{
	if (!HasAuthority() || bIsFakeDead || bIsDead) return;
	bIsFakeDead = true;
	ApplyFakeDeathPhysics(true);
}

void ABaseCharacter::StopFakeDeath()
{
	if (!HasAuthority() || !bIsFakeDead) return;
	bIsFakeDead = false;
	ApplyFakeDeathPhysics(false);
}

void ABaseCharacter::OnRep_IsFakeDead()
{
	ApplyFakeDeathPhysics(bIsFakeDead);
}

void ABaseCharacter::ApplyFakeDeathPhysics(bool bActivate)
{
	if (bActivate)
	{
		// 밟고 있던 발판과 공중 여부를 움직임 비활성화 전에 기억한다. (DisableMovement 가 베이스를 지운다)
		UPrimitiveComponent* FakeDeathBase = nullptr;
		bool bFakeDiedInAir = false;
		if (GetCharacterMovement())
		{
			FakeDeathBase = GetCharacterMovement()->GetMovementBase();
			bFakeDiedInAir = GetCharacterMovement()->IsFalling();
			GetCharacterMovement()->StopMovementImmediately();
			GetCharacterMovement()->DisableMovement();
		}

		if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (GetMesh())
		{
			GetMesh()->SetSimulatePhysics(true);
			GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		}
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
			// 가짜 시체 중에도 해제 버튼(HUD)을 클릭할 수 있게 커서를 켠다.
			// (DisableInput 은 UMG 클릭을 막지 않고, Alt 키 입력만 막는다)
			if (ABasePlayerController* BPC = Cast<ABasePlayerController>(PC))
			{
				BPC->SetHUDCursorMode(true);
			}
		}

		// 진짜 사망과 동일하게 열차에 부착한다. 안 붙이면 움직임이 꺼진 액터(캡슐+카메라)가
		// 월드 좌표에 남고 열차만 달려나가, 카메라가 몸에서 떨어진 것처럼 보인다.
		AttachRagdollToBase(FakeDeathBase, bFakeDiedInAir);
	}
	else
	{
		// 부착 대기 타이머가 남아 있으면 일어난 뒤에 뒤늦게 부착되므로 먼저 취소한다.
		GetWorldTimerManager().ClearTimer(CorpseAttachTimer);

		// 래그돌이 쓰러진 위치(골반)로 캡슐을 옮긴 뒤 일어난다.
		// 안 그러면 죽은 척을 시작한 지점에 그대로 서게 된다.
		if (USkeletalMeshComponent* M = GetMesh())
		{
			const FName PelvisBone = (M->GetBoneIndex(RagdollPelvisBone) != INDEX_NONE)
				? RagdollPelvisBone : M->GetBoneName(0);
			const FVector PelvisLocation = M->GetBoneLocation(PelvisBone);
			const float HalfHeight = GetCapsuleComponent()
				? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;

			M->SetSimulatePhysics(false);
			M->SetCollisionProfileName(TEXT("CharacterMesh"));

			// 열차에 붙어 있던 상태를 풀고, 캡슐을 시체 위치로 맞춘다. (서버 권위, 위치는 복제된다)
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			if (HasAuthority())
			{
				SetActorLocation(PelvisLocation + FVector(0.f, 0.f, HalfHeight), /*bSweep=*/false,
					nullptr, ETeleportType::TeleportPhysics);
			}

			M->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			M->SetRelativeLocationAndRotation(DefaultMeshRelativeLocation, DefaultMeshRelativeRotation);
		}

		if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (GetCharacterMovement()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			EnableInput(PC);
			if (ABasePlayerController* BPC = Cast<ABasePlayerController>(PC))
			{
				BPC->SetHUDCursorMode(false);
			}
		}
	}
}

void ABaseCharacter::StealAmmo(ABaseCharacter* DeadCharacter)
{
	if (!HasAuthority() || !IsValid(DeadCharacter) || !DeadCharacter->IsDead()) return;

	// 탄약 단일 소스 = CurrentAmmo 어트리뷰트 (GA_FireGun 소모와 동일 소스 — PS.AmmoCount 사용 안 함).
	UAbilitySystemComponent* MyASC   = GetAbilitySystemComponent();
	UAbilitySystemComponent* DeadASC = DeadCharacter->GetAbilitySystemComponent();
	if (!MyASC || !DeadASC) return;

	const float DeadAmmo = DeadASC->GetNumericAttribute(UBaseAttributeSet::GetCurrentAmmoAttribute());
	if (DeadAmmo <= 0.f) return;

	// 내 MaxAmmo 는 PreAttributeChange 에서 클램프된다.
	MyASC->ApplyModToAttribute(UBaseAttributeSet::GetCurrentAmmoAttribute(), EGameplayModOp::Additive, DeadAmmo);
	DeadASC->SetNumericAttributeBase(UBaseAttributeSet::GetCurrentAmmoAttribute(), 0.f);
}

// ============================================================
// 역할별 능력 — Mechanic
// ============================================================

bool ABaseCharacter::CanInstantFixGear() const
{
	return CharacterTag == Character::Crew::Mechanic.GetTag();
}

// ============================================================
// 역할별 능력 — Watchman
// ============================================================

void ABaseCharacter::ToggleLantern()
{
	if (!HasAuthority()) return;

	// 순찰자가 아니면 켜지지 않게 (GA 태그 조건 외 이중 방어). 끄는 것은 역할과 무관하게 허용.
	if (!bLanternOn && CharacterTag != Character::Crew::Watchman.GetTag()) return;

	bLanternOn = !bLanternOn;
	OnRep_LanternOn();
}

void ABaseCharacter::OnRep_LanternOn()
{
	// 빛으로 순찰자 역할이 유추되면 안 되므로 본인 화면에서만 보인다.
	if (LanternLight) LanternLight->SetVisibility(bLanternOn && !bMeshHidden && IsLocallyControlled());
}

void ABaseCharacter::SetTrackedPlayers(ABaseCharacter* P1, ABaseCharacter* P2,
	FLinearColor C1, FLinearColor C2)
{
	TrackedPlayer1 = P1;
	TrackedPlayer2 = P2;
	TrackColor1    = C1;
	TrackColor2    = C2;

	Footprints1.Reset();
	Footprints2.Reset();
}

void ABaseCharacter::ActivateFootprintVision()
{
	if (!HasAuthority()) return;

	TArray<FVector> Pos1, Pos2;
	for (const FFootprintRecord& R : Footprints1) Pos1.Add(R.Location);
	for (const FFootprintRecord& R : Footprints2) Pos2.Add(R.Location);

	Client_ReceiveFootprints(Pos1, TrackColor1, Pos2, TrackColor2);
}

void ABaseCharacter::RecordFootprintPositions()
{
	const float Now = GetWorld()->GetTimeSeconds();

	if (TrackedPlayer1.IsValid() && !TrackedPlayer1->IsDead())
	{
		FFootprintRecord R;
		R.Location  = TrackedPlayer1->GetActorLocation();
		R.Timestamp = Now;
		Footprints1.Add(R);
	}

	if (TrackedPlayer2.IsValid() && !TrackedPlayer2->IsDead())
	{
		FFootprintRecord R;
		R.Location  = TrackedPlayer2->GetActorLocation();
		R.Timestamp = Now;
		Footprints2.Add(R);
	}

	PruneOldRecords(Footprints1);
	PruneOldRecords(Footprints2);
}

void ABaseCharacter::PruneOldRecords(TArray<FFootprintRecord>& Records)
{
	const float Cutoff = GetWorld()->GetTimeSeconds() - FootprintRecordDuration;
	Records.RemoveAll([Cutoff](const FFootprintRecord& R) { return R.Timestamp < Cutoff; });
}

void ABaseCharacter::Client_ReceiveFootprints_Implementation(
	const TArray<FVector>& Positions1, FLinearColor InColor1,
	const TArray<FVector>& Positions2, FLinearColor InColor2)
{
	SpawnFootprintDecals(Positions1, InColor1);
	SpawnFootprintDecals(Positions2, InColor2);
}

void ABaseCharacter::SpawnFootprintDecals(const TArray<FVector>& Positions, FLinearColor Color)
{
	if (!FootprintDecalMaterial) return;

	for (const FVector& Pos : Positions)
	{
		UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
			GetWorld(), FootprintDecalMaterial, DecalSize,
			Pos, FRotator(-90.f, 0.f, 0.f), DecalLifespan);

		if (!Decal) continue;

		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(FootprintDecalMaterial, this);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("FootprintColor"), Color);
			Decal->SetMaterial(0, MID);
		}
	}
}

// ============================================================
// 역할별 능력 — Stoker
// ============================================================

bool ABaseCharacter::IsHeatImmune() const
{
	return CharacterTag == Character::Crew::Stoker.GetTag();
}

void ABaseCharacter::ForceClosePressureValve(AActor* PressureValveActor)
{
	if (!HasAuthority() || !PressureValveActor) return;

	if (APressureValve* Valve = Cast<APressureValve>(PressureValveActor))
	{
		Valve->ForceStop();
	}
}

// ============================================================
// 역할별 능력 — Porter
// ============================================================

float ABaseCharacter::GetHeavyCarrySpeedPenalty() const
{
	// 짐꾼은 무게 패널티 없음. 다른 역할은 RecalculateMoveSpeed의 WeightSpeedPenaltyPerUnit 으로 처리.
	return 0.f;
}

float ABaseCharacter::GetMaxCarryWeight() const
{
	return MaxCarryWeight;
}

// ============================================================
// Input
// ============================================================
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABaseCharacter::HandleJumpInput);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABaseCharacter::Look);
	
		EnhancedInputComponent->BindAction(ClimbAction, ETriggerEvent::Started, this, &ABaseCharacter::OnClimbActionStarted);
	}

	// 기어 QTE(화살표) — 새 IA 에셋 없이 BasePlayerController와 동일한
	// 레거시 raw key 바인딩 방식 사용. 화살표는 WASD/Enhanced Input과 겹치지 않는다.
	// 압력밸브 정지(스페이스)는 IA_Jump 가 스페이스바 입력을 소비해 raw 바인딩으로는
	// 전달되지 않으므로 HandleJumpInput 에서 relay 한다.
	PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ABaseCharacter::OnQTEUpPressed);
	PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ABaseCharacter::OnQTEDownPressed);
	PlayerInputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ABaseCharacter::OnQTELeftPressed);
	PlayerInputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ABaseCharacter::OnQTERightPressed);

	// 손에 든 아이템 버리기.
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &ABaseCharacter::DropHeldItem);
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	if (!CustomMovementComponent)
	{
		return;
	}

	if (CustomMovementComponent->IsClimbing())
	{
		HandleClimbMovementInput(Value);
	}
	else
	{
		HandleGroundMovementInput(Value);
	}
	if (bInputLockedByMinigame) return;
}

void ABaseCharacter::HandleGroundMovementInput(const FInputActionValue& Value)
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

void ABaseCharacter::HandleClimbMovementInput(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	const FVector ForwardDirection = FVector::CrossProduct(
		-CustomMovementComponent->GetClimbableSurfaceNormal(),
		GetActorRightVector()
	);

	const FVector RightDirection = FVector::CrossProduct(
		-CustomMovementComponent->GetClimbableSurfaceNormal(),
		-GetActorUpVector()
	);

	// add movement 
	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ABaseCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// 설정창 마우스 감도 / Y축 반전 반영.
	if (const UGODGameUserSettings* Settings = UGODGameUserSettings::Get())
	{
		LookAxisVector *= Settings->MouseSensitivity;
		if (Settings->bInvertYAxis)
		{
			LookAxisVector.Y = -LookAxisVector.Y;
		}
	}

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABaseCharacter::OnClimbActionStarted(const FInputActionValue& Value)
{
	if (!CustomMovementComponent)
	{
		return;
	}


	if (!CustomMovementComponent->IsClimbing())
	{
		CustomMovementComponent->ToggleClimbing(true);
		if (CustomMovementComponent->IsClimbing())
		{
			PlayCharacterSoundLocal(SoundRows::FootstepClimb);
		}
	}
	else
	{
		CustomMovementComponent->ToggleClimbing(false);
	}
}

void ABaseCharacter::HandleJumpInput()
{
	// 압력밸브 미니게임 중에는 스페이스바를 점프 대신 "바늘 정지" 입력으로 사용한다.
	if (bInputLockedByMinigame)
	{
		if (ActivePressureValve.IsValid())
		{
			Server_SubmitValveStop();
		}
		return;
	}
	Jump();
}

void ABaseCharacter::OnQTEUpPressed()
{
	if (ActiveGearQTESlot.IsValid()) Server_SubmitQTEDirection(EQTEDirection::Up);
}

void ABaseCharacter::OnQTEDownPressed()
{
	if (ActiveGearQTESlot.IsValid()) Server_SubmitQTEDirection(EQTEDirection::Down);
}

void ABaseCharacter::OnQTELeftPressed()
{
	if (ActiveGearQTESlot.IsValid()) Server_SubmitQTEDirection(EQTEDirection::Left);
}

void ABaseCharacter::OnQTERightPressed()
{
	if (ActiveGearQTESlot.IsValid()) Server_SubmitQTEDirection(EQTEDirection::Right);
}

void ABaseCharacter::Server_SubmitQTEDirection_Implementation(EQTEDirection Dir)
{
	if (ActiveGearQTESlot.IsValid())
	{
		ActiveGearQTESlot->SubmitQTEInput(Dir);
	}
}

void ABaseCharacter::Server_SubmitValveStop_Implementation()
{
	if (ActivePressureValve.IsValid())
	{
		ActivePressureValve->SubmitStopInput();
	}
}

void ABaseCharacter::Client_StartGearQTE_Implementation(AGearSlot* Slot)
{
	ActiveGearQTESlot = Slot;
	bInputLockedByMinigame = true;

	// 미니게임음은 본인만 듣는 UI 성 사운드라 2D 로컬 재생.
	UGameSoundStatics::PlaySound2DFromTable(this, CharacterSoundTable, SoundRows::QTEStart);

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		if (AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD()))
		{
			HUD->ShowGearQTE(Slot);
		}
	}
}

void ABaseCharacter::Client_EndGearQTE_Implementation(bool bSuccess)
{
	ActiveGearQTESlot = nullptr;
	bInputLockedByMinigame = false;

	UGameSoundStatics::PlaySound2DFromTable(this, CharacterSoundTable,
		bSuccess ? SoundRows::QTESuccess : SoundRows::QTEFail);

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		if (AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD()))
		{
			HUD->HideGearQTE(bSuccess);
		}
	}
}

void ABaseCharacter::Client_StartPressureMinigame_Implementation(APressureValve* Valve)
{
	ActivePressureValve = Valve;
	bInputLockedByMinigame = true;

	UGameSoundStatics::PlaySound2DFromTable(this, CharacterSoundTable, SoundRows::ValveStart);

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		if (AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD()))
		{
			HUD->ShowPressureMinigame(Valve);
		}
	}
}

void ABaseCharacter::Client_EndPressureMinigame_Implementation(bool bSuccess)
{
	ActivePressureValve = nullptr;
	bInputLockedByMinigame = false;

	UGameSoundStatics::PlaySound2DFromTable(this, CharacterSoundTable,
		bSuccess ? SoundRows::ValveSuccess : SoundRows::ValveFail);

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		if (AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD()))
		{
			HUD->HidePressureMinigame(bSuccess);
		}
	}
}
