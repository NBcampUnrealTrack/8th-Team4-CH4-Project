// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Role/GODCharacterWatchman.h"
#include "Game/BaseGameplayTags.h"
#include "Components/SpotLightComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AGODCharacterWatchman::AGODCharacterWatchman()
{
	CharacterTag = Character::Crew::Watchman;

	LanternLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(GetMesh(), TEXT("hand_l"));
	LanternLight->SetVisibility(false);
	LanternLight->Intensity = 3000.f;
	LanternLight->OuterConeAngle = 35.f;
}

void AGODCharacterWatchman::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(FootprintRecordTimer, this,
			&AGODCharacterWatchman::RecordFootprintPositions,
			RecordInterval, true);
	}
}

void AGODCharacterWatchman::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FootprintRecordTimer);
	Super::EndPlay(EndPlayReason);
}

void AGODCharacterWatchman::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGODCharacterWatchman, bLanternOn);
}

// ── 랜턴 ──

void AGODCharacterWatchman::ToggleLantern()
{
	if (!HasAuthority()) return;
	bLanternOn = !bLanternOn;
	OnRep_LanternOn();
}

void AGODCharacterWatchman::OnRep_LanternOn()
{
	LanternLight->SetVisibility(bLanternOn);
}

// ── 발자국 추적 ──

void AGODCharacterWatchman::SetTrackedPlayers(ABaseCharacter* InPlayer1, ABaseCharacter* InPlayer2,
	FLinearColor InColor1, FLinearColor InColor2)
{
	TrackedPlayer1 = InPlayer1;
	TrackedPlayer2 = InPlayer2;
	TrackColor1 = InColor1;
	TrackColor2 = InColor2;

	Footprints1.Reset();
	Footprints2.Reset();
}

void AGODCharacterWatchman::ActivateFootprintVision()
{
	if (!HasAuthority()) return;

	TArray<FVector> Pos1, Pos2;
	for (const FFootprintRecord& R : Footprints1) Pos1.Add(R.Location);
	for (const FFootprintRecord& R : Footprints2) Pos2.Add(R.Location);

	Client_ReceiveFootprints(Pos1, TrackColor1, Pos2, TrackColor2);
}

void AGODCharacterWatchman::RecordFootprintPositions()
{
	const float Now = GetWorld()->GetTimeSeconds();

	if (TrackedPlayer1.IsValid() && !TrackedPlayer1->IsDead())
		Footprints1.Add({ TrackedPlayer1->GetActorLocation(), Now });

	if (TrackedPlayer2.IsValid() && !TrackedPlayer2->IsDead())
		Footprints2.Add({ TrackedPlayer2->GetActorLocation(), Now });

	PruneOldRecords(Footprints1);
	PruneOldRecords(Footprints2);
}

void AGODCharacterWatchman::PruneOldRecords(TArray<FFootprintRecord>& Records)
{
	const float Cutoff = GetWorld()->GetTimeSeconds() - FootprintRecordDuration;
	Records.RemoveAll([Cutoff](const FFootprintRecord& R) { return R.Timestamp < Cutoff; });
}

void AGODCharacterWatchman::Client_ReceiveFootprints_Implementation(
	const TArray<FVector>& Positions1, FLinearColor InColor1,
	const TArray<FVector>& Positions2, FLinearColor InColor2)
{
	SpawnFootprintDecals(Positions1, InColor1);
	SpawnFootprintDecals(Positions2, InColor2);
}

void AGODCharacterWatchman::SpawnFootprintDecals(const TArray<FVector>& Positions, FLinearColor Color)
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
