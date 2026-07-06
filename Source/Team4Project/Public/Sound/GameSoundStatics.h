// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameSoundStatics.generated.h"

class UDataTable;
class UAudioComponent;
class USceneComponent;
class USoundBase;
struct FGameSoundRow;

/**
 * 사운드 DT(FGameSoundRow) 공용 재생 헬퍼.
 * 소비자(캐릭터/문/위젯/게임스테이트)는 에디터에서 DT 하나만 지정하고 행 이름으로 재생한다.
 * 모든 함수는 호출한 머신에서만 재생 — 전 클라 재생이 필요하면 호출측에서 멀티캐스트로 감싼다
 * (예: ABaseCharacter::Multicast_PlayCharacterSound).
 */
UCLASS()
class TEAM4PROJECT_API UGameSoundStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 2D 단발 재생 (UI 클릭, 경고음, 승패 등 위치와 무관한 소리).
	UFUNCTION(BlueprintCallable, Category = "Sound", meta = (WorldContext = "WorldContextObject"))
	static void PlaySound2DFromTable(const UObject* WorldContextObject, const UDataTable* SoundTable, FName RowName);

	// 3D 위치 단발 재생 (발소리, 총성, 문 등).
	UFUNCTION(BlueprintCallable, Category = "Sound", meta = (WorldContext = "WorldContextObject"))
	static void PlaySoundAtLocationFromTable(const UObject* WorldContextObject, const UDataTable* SoundTable, FName RowName, FVector Location);

	// 2D 지속 재생 (BGM). 반환된 AudioComponent 로 Stop/FadeOut 제어. 루프는 사운드 애셋에서 설정.
	UFUNCTION(BlueprintCallable, Category = "Sound", meta = (WorldContext = "WorldContextObject"))
	static UAudioComponent* SpawnSound2DFromTable(const UObject* WorldContextObject, const UDataTable* SoundTable, FName RowName);

	// 컴포넌트 부착 지속 재생 (기차 운행음 등 움직이는 지속음).
	UFUNCTION(BlueprintCallable, Category = "Sound")
	static UAudioComponent* SpawnSoundAttachedFromTable(const UDataTable* SoundTable, FName RowName, USceneComponent* AttachToComponent, FName SocketName = NAME_None);

	// 행에서 사운드 하나 선택해 반환 (여러 개면 무작위). 재생을 직접 제어하고 싶을 때 사용.
	UFUNCTION(BlueprintCallable, Category = "Sound")
	static USoundBase* GetSoundFromTable(const UDataTable* SoundTable, FName RowName);

private:
	static const FGameSoundRow* FindRow(const UDataTable* SoundTable, FName RowName);
	static USoundBase* PickSound(const FGameSoundRow& Row);
};
