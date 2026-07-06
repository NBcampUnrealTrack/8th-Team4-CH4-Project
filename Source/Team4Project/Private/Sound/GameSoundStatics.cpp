// Fill out your copyright notice in the Description page of Project Settings.

#include "Sound/GameSoundStatics.h"
#include "Sound/GameSoundTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/DataTable.h"

const FGameSoundRow* UGameSoundStatics::FindRow(const UDataTable* SoundTable, FName RowName)
{
	if (!SoundTable || RowName.IsNone())
	{
		return nullptr;
	}
	// 행이 없으면 경고 로그 — DT 에 행을 추가해야 한다는 신호.
	return SoundTable->FindRow<FGameSoundRow>(RowName, TEXT("GameSound"));
}

USoundBase* UGameSoundStatics::PickSound(const FGameSoundRow& Row)
{
	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : Row.Sounds)
	{
		if (Sound)
		{
			ValidSounds.Add(Sound);
		}
	}
	if (ValidSounds.Num() == 0)
	{
		return nullptr;
	}
	return ValidSounds[FMath::RandHelper(ValidSounds.Num())];
}

USoundBase* UGameSoundStatics::GetSoundFromTable(const UDataTable* SoundTable, FName RowName)
{
	const FGameSoundRow* Row = FindRow(SoundTable, RowName);
	return Row ? PickSound(*Row) : nullptr;
}

void UGameSoundStatics::PlaySound2DFromTable(const UObject* WorldContextObject, const UDataTable* SoundTable, FName RowName)
{
	const FGameSoundRow* Row = FindRow(SoundTable, RowName);
	USoundBase* Sound = Row ? PickSound(*Row) : nullptr;
	if (!Sound)
	{
		return;
	}
	UGameplayStatics::PlaySound2D(WorldContextObject, Sound, Row->VolumeMultiplier, Row->PitchMultiplier);
}

void UGameSoundStatics::PlaySoundAtLocationFromTable(const UObject* WorldContextObject, const UDataTable* SoundTable, FName RowName, FVector Location)
{
	const FGameSoundRow* Row = FindRow(SoundTable, RowName);
	USoundBase* Sound = Row ? PickSound(*Row) : nullptr;
	if (!Sound)
	{
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(WorldContextObject, Sound, Location,
		Row->VolumeMultiplier, Row->PitchMultiplier, 0.f, Row->Attenuation);
}

UAudioComponent* UGameSoundStatics::SpawnSound2DFromTable(const UObject* WorldContextObject, const UDataTable* SoundTable, FName RowName)
{
	const FGameSoundRow* Row = FindRow(SoundTable, RowName);
	USoundBase* Sound = Row ? PickSound(*Row) : nullptr;
	if (!Sound)
	{
		return nullptr;
	}
	return UGameplayStatics::SpawnSound2D(WorldContextObject, Sound, Row->VolumeMultiplier, Row->PitchMultiplier);
}

UAudioComponent* UGameSoundStatics::SpawnSoundAttachedFromTable(const UDataTable* SoundTable, FName RowName, USceneComponent* AttachToComponent, FName SocketName)
{
	const FGameSoundRow* Row = FindRow(SoundTable, RowName);
	USoundBase* Sound = Row ? PickSound(*Row) : nullptr;
	if (!Sound || !AttachToComponent)
	{
		return nullptr;
	}
	return UGameplayStatics::SpawnSoundAttached(Sound, AttachToComponent, SocketName,
		FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false,
		Row->VolumeMultiplier, Row->PitchMultiplier, 0.f, Row->Attenuation);
}
