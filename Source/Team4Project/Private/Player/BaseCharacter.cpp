// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/BaseCharacter.h"
#include "Player/GODPlayerState.h"
#include "Player/VoiceChannelSubsystem.h"
#include "Player/BasePlayerController.h"
#include "Player/Component/BaseAbilitySystemComponent.h"
#include "Player/Component/BaseAttributeSet.h"
#include "Component/InteractComponent.h"
#include "InteractiveProp/ItemBase.h"
#include "InteractiveProp/DoorBase.h"
#include "InteractiveProp/PressureValve.h"
#include "InteractiveProp/GearSlot.h"
#include "Quest/QuestStation.h"
#include "Meeting/MeetingRoom.h"
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
#include "UI/HUD/GODMainHUDWidget.h"
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
	LanternLight->Intensity = 3000.f;
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

	if (GetMesh())
	{
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

	// GameMode 풀에서 색상 변형 배정 (리스폰 시 같은 플레이어는 같은 색 유지
	if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
	{
		SkinVariantIndex = GM->AssignSkinVariant(
			GetPlayerState(), InSkinIndex, SkinOptions[InSkinIndex].VariantTextures.Num());
	}
	ApplySkin(); // 리슨 서버(호스트) 화면 반영 — 클라는 OnRep 에서 반영
}

void ABaseCharacter::OnRep_SkinVariantIndex()
{
	// SkinIndex 와 도착 순서가 다를 수 있으므로 여기서도 적용 (ApplySkin 은
	ApplySkin();
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

	if (Skin.VariantTextures.IsValidIndex(SkinVariantIndex) && Skin.VariantTextures[SkinVariantIndex])
	{
		if (UMaterialInstanceDynamic* MID = MeshComp->CreateDynamicMaterialInstance(0))
		{
			MID->SetTextureParameterValue(TEXT("BaseColorTexture"), Skin.VariantTextures[SkinVariantIndex]);
		}
	}

	if (CustomMovementComponent)
	{
		CustomMovementComponent->IdleToClimbMontage = Skin.IdleToClimbMontage;
		CustomMovementComponent->ClimbToTopMontage = Skin.ClimbToTopMontage;
		CustomMovementComponent->ClimbDownLedgeMontage = Skin.ClimbDownLedgeMontage;

		CustomMovementComponent->RefreshAnimInstance();
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
	DOREPLIFETIME(ABaseCharacter, bIsWorkingOnQuest);
	DOREPLIFETIME(ABaseCharacter, CurrentHeldItem);
	DOREPLIFETIME(ABaseCharacter, bMeshHidden);
	DOREPLIFETIME(ABaseCharacter, SkinIndex);
	DOREPLIFETIME(ABaseCharacter, SkinVariantIndex);
	// 직업 태그는 소유 클라에만 복제(다른 플레이어에게 역할 노출 방지).
	DOREPLIFETIME_CONDITION(ABaseCharacter, CharacterTag, COND_OwnerOnly);

	DOREPLIFETIME(ABaseCharacter, CurrentCoal);

	// 역할별 상태
	DOREPLIFETIME(ABaseCharacter, bIsInVent);
	DOREPLIFETIME(ABaseCharacter, bIsInvisible);
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
// 장착 슬롯 (물리 아이템, 한 번에 하나만)
// ============================================================

void ABaseCharacter::ClearEquipSlot()
{
	if (!HasAuthority()) return;

	// 물리 아이템을 들고 있으면 떨군다.
	// Server_Drop 이 자체적으로 장착 태그/CurrentHeldItem 까지 정리한다.
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

		//아이템 Drop 몽타주
		Multicast_PlayDropMontage();
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
// 퀘스트
// ============================================================
void ABaseCharacter::SetActiveQuestStation(AQuestStation* InStation)
{
	if (!HasAuthority()) return;

	ActiveQuestStation = InStation;

	// 작업 자세는 전 클라에 보여야함
	bIsWorkingOnQuest = (InStation != nullptr);
	OnRep_IsWorkingOnQuest();
}

void ABaseCharacter::OnRep_IsWorkingOnQuest()
{
	// ABP 가 bIsWorkingOnQuest 를 직접 읽음
}

void ABaseCharacter::Client_StartQuestMinigame_Implementation(AQuestStation* Station)
{
	ActiveQuestStation = Station;
	SetQuestUIOpen(true);

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		if (AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD()))
		{
			HUD->ShowQuestMinigame(Station);
		}
	}
}

void ABaseCharacter::Client_EndQuestMinigame_Implementation(bool bSuccess)
{
	ActiveQuestStation = nullptr;
	SetQuestUIOpen(false);

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetController()))
	{
		if (AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD()))
		{
			HUD->HideQuestMinigame(bSuccess);
		}
	}
}

void ABaseCharacter::SetQuestUIOpen(bool bOpen)
{
	bQuestUIOpen = bOpen;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;

	PC->SetShowMouseCursor(bOpen);

	if (bOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void ABaseCharacter::Server_CompleteQuest_Implementation()
{
	AQuestStation* Station = ActiveQuestStation.Get();
	if (!Station || Station->GetQuestPlayer() != this || bIsDead) return;

	if (!Station->HasMinDurationElapsed())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Quest] %s 가 최소 소요 시간 전에 완료를 보고해 거부됨 (%s)"),
			*GetName(), *Station->QuestId.ToString());
		return;
	}

	Station->ReleaseQuestPlayer();

	// 배정 여부 판정과 보상은 게임모드
	if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
	{
		GM->HandleQuestCompleted(GetPlayerState<AGODPlayerState>(), Station);
	}

	Client_EndQuestMinigame(true);
}

void ABaseCharacter::Server_CancelQuest_Implementation()
{
	if (AQuestStation* Station = ActiveQuestStation.Get())
	{
		Station->AbortQuest();
	}
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
	// 죽은 캐릭터는 상호작용 대상이 아니다 (탄약 빼앗기는 총 시스템과 함께 제거됨).
	if (!HasAuthority() || !Interactor || bIsDead) return;

	FGameplayEventData EventData;
	EventData.OptionalObject = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		Interactor, FGameplayTag::RequestGameplayTag(TEXT("Ability.Trigger.SearchTarget")), EventData);
}

FText ABaseCharacter::GetInteractPrompt_Implementation() const
{
	return bIsDead ? FText::GetEmpty() : FText::FromString(TEXT("수색"));
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

	if (CharacterTag == Character::Crew::Watchman.GetTag())
	{
		GetWorldTimerManager().SetTimer(
			FootprintRecordTimer, this, &ABaseCharacter::RecordFootprintPositions,
			RecordInterval, /*bLoop=*/true);
	}
}

void ABaseCharacter::DeactivateRoleTimers()
{
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

	// 회의 중에는 사망 판정이 없다. 열차 밖으로 떨어졌으면 회의실 좌석으로 복귀만 시킨다.
	if (GS->CurrentPhase == EGamePhase::Meeting)
	{
		float MeetingRefZ = 0.f;
		if (GetFallReferenceZ(MeetingRefZ) &&
			GetActorLocation().Z < MeetingRefZ - FallDeathDepth)
		{
			if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
			{
				GM->RescueToMeeting(this);
			}
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
		// 사망 시스템 제거(2026-07-13): 낙하는 사망이 아니라 FallRespawnDelay 뒤 열차 복귀.
		if (!GetWorldTimerManager().IsTimerActive(TrainRescueTimer))
		{
			GetWorldTimerManager().SetTimer(
				TrainRescueTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
					{
						RescueToTrain();
					}),
				FallRespawnDelay, /*bLoop=*/false);
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

bool ABaseCharacter::RescueToTrain()
{
	GetWorldTimerManager().ClearTimer(TrainRescueTimer);

	if (!HasAuthority() || bIsDead) return false;

	// 회의실(열차 자식)이 있으면 그 좌석으로 — 확실한 실내 지점이라 안전하다.
	for (TActorIterator<AMeetingRoom> It(GetWorld()); It; ++It)
	{
		(*It)->TeleportToSeat(this, FMath::RandRange(0, 7));
		return true;
	}

	// 폴백: 열차 액터 기준 오프셋. (회의실을 배치하면 이 경로는 안 탄다)
	if (IsValid(CachedTrain))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Rescue] MeetingRoom 이 없어 열차 기준 오프셋으로 복귀한다. 회의실 배치 권장."));

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}

		const FVector Loc = CachedTrain->GetActorLocation() + TrainRespawnOffset;
		if (!TeleportTo(Loc, GetActorRotation()))
		{
			SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
		}
		return true;
	}

	// 열차도 없으면(비정상 상황) 스타트 지점으로라도.
	return RescueToStart();
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

		// 회의 중에는 회의실로 복귀.
		if (GS->CurrentPhase == EGamePhase::Meeting)
		{
			if (AGODGameMode* GM = GetWorld()->GetAuthGameMode<AGODGameMode>())
			{
				GM->RescueToMeeting(this);
				return;
			}
		}

		// 진행 중: 사망 시스템 제거(2026-07-13) — KillZ 까지 떨어졌으면 즉시 열차로 복귀.
		if (GS->CurrentPhase == EGamePhase::Playing && RescueToTrain())
		{
			return;
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
	TrackColor1 = C1;
	TrackColor2 = C2;

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
		R.Location = TrackedPlayer1->GetActorLocation();
		R.Timestamp = Now;
		Footprints1.Add(R);
	}

	if (TrackedPlayer2.IsValid() && !TrackedPlayer2->IsDead())
	{
		FFootprintRecord R;
		R.Location = TrackedPlayer2->GetActorLocation();
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

	// 액티브 스킬 키보드 발동 — 1/2 = 액티브 1/2, 세 번째는 R
	// (3키는 강제 시작에 쓰이고 있어 비워 둔다). 버튼 클릭 방식과 병행.
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ABaseCharacter::OnAbilitySlot1Pressed);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ABaseCharacter::OnAbilitySlot2Pressed);
	PlayerInputComponent->BindKey(EKeys::R,   IE_Pressed, this, &ABaseCharacter::OnAbilitySlot3Pressed);
}

void ABaseCharacter::OnAbilitySlot1Pressed() { ActivateAbilitySlotFromKey(0); }
void ABaseCharacter::OnAbilitySlot2Pressed() { ActivateAbilitySlotFromKey(1); }
void ABaseCharacter::OnAbilitySlot3Pressed() { ActivateAbilitySlotFromKey(2); }

void ABaseCharacter::ActivateAbilitySlotFromKey(int32 SlotIndex)
{
	// 퀘스트 팝업이 열려 있는 동안은 이동과 마찬가지로 스킬 입력도 막는다.
	if (bQuestUIOpen) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	AGODHUD* HUD = Cast<AGODHUD>(PC->GetHUD());
	if (!HUD || !HUD->MainHUDWidget) return;

	HUD->MainHUDWidget->TryActivateActiveSlot(SlotIndex);
}

void ABaseCharacter::Move(const FInputActionValue& Value)
{
	if (!CustomMovementComponent)
	{
		return;
	}

	// 잠금 검사가 이동 처리 뒤에 있어서 아무 효과가 없었다. 반드시 먼저 걸러야 한다.
	if (bQuestUIOpen) return;

	if (CustomMovementComponent->IsClimbing())
	{
		HandleClimbMovementInput(Value);
	}
	else
	{
		HandleGroundMovementInput(Value);
	}
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
	// 팝업이 열려 있는 동안엔 마우스가 커서로 쓰이므로 시점이 돌면 안 된다.
	if (bQuestUIOpen) return;

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

void ABaseCharacter::Multicast_PlayDoorMontage_Implementation(bool bIsOpening)
{
	if (SkinOptions.IsValidIndex(SkinIndex))
	{
		// bIsOpening이 true면 여는 몽타주, false면 닫는 몽타주 선택
		UAnimMontage* MontageToPlay = bIsOpening ? SkinOptions[SkinIndex].DoorOpenMontage
			: SkinOptions[SkinIndex].DoorCloseMontage;

		PlayMontageLocal(MontageToPlay);
	}
}

void ABaseCharacter::Multicast_PlayMatchEndMontage_Implementation(bool bIsVictory)
{
	if (!SkinOptions.IsValidIndex(SkinIndex)) return;

	UAnimMontage* MontageToPlay = bIsVictory ? SkinOptions[SkinIndex].VictoryMontage
		: SkinOptions[SkinIndex].DefeatMontage;

	PlayMontageLocal(MontageToPlay);
}

void ABaseCharacter::Multicast_PlayPickUpMontage_Implementation()
{
	if (SkinOptions.IsValidIndex(SkinIndex))
	{
		PlayMontageLocal(SkinOptions[SkinIndex].PickUpMontage);
	}
}

void ABaseCharacter::Multicast_PlayDropMontage_Implementation()
{
	if (SkinOptions.IsValidIndex(SkinIndex))
	{
		PlayMontageLocal(SkinOptions[SkinIndex].DropMontage);
	}
}

void ABaseCharacter::Client_ForceStartClimbing_Implementation()
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