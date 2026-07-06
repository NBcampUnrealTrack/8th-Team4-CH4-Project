// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameSoundTypes.generated.h"

class USoundBase;
class USoundAttenuation;

/**
 * 사운드 DT 공용 행 구조체.
 * 프로젝트의 사운드 DT 3개(캐릭터/UI/게임 전체)가 전부 이 구조체를 사용한다.
 * 행 이름(Row Name)이 곧 사운드 키 — C++ 은 아래 SoundRows 상수를, BP 는 같은 문자열을 사용.
 */
USTRUCT(BlueprintType)
struct FGameSoundRow : public FTableRowBase
{
	GENERATED_BODY()

	// 재생할 사운드. 2개 이상 넣으면 재생할 때마다 무작위로 하나 선택 (발소리 변주 등).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TArray<TObjectPtr<USoundBase>> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	float VolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	float PitchMultiplier = 1.f;

	// 3D 재생 시 감쇠 설정. 비우면 사운드 애셋 자체 설정을 따른다 (2D 재생에는 미사용).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundAttenuation> Attenuation;
};

/**
 * DT 행 이름 상수. 에디터에서 만드는 DT 의 행 이름과 반드시 일치해야 한다.
 * (여기 없는 사운드는 상수를 추가하거나, BP 에서 같은 규칙의 문자열로 사용)
 */
namespace SoundRows
{
	// ── 캐릭터 사운드 DT (DT_CharacterSounds) — BaseCharacter / DoorBase / GearSlot / SinkBase 등에 지정 ──
	inline const FName FootstepWalk(TEXT("Footstep.Walk"));
	inline const FName FootstepJump(TEXT("Footstep.Jump"));
	inline const FName FootstepLand(TEXT("Footstep.Land"));
	inline const FName FootstepClimb(TEXT("Footstep.Climb"));
	inline const FName GunFire(TEXT("Gun.Fire"));
	inline const FName GunReload(TEXT("Gun.Reload"));
	inline const FName GunEquip(TEXT("Gun.Equip"));
	inline const FName GunUnequip(TEXT("Gun.Unequip"));
	inline const FName DoorOpen(TEXT("Door.Open"));
	inline const FName DoorClose(TEXT("Door.Close"));
	inline const FName ItemPickup(TEXT("Item.Pickup"));
	inline const FName ItemDrop(TEXT("Item.Drop"));
	// 사망 래그돌음. 무법자 "죽은 척"도 진짜와 구분되지 않도록 같은 행을 재사용한다.
	inline const FName CharacterDie(TEXT("Character.Die"));

	// 능력 사용음 — 역할 노출 방지를 위해 전부 본인 로컬 재생 (Client_PlayCharacterSound)
	inline const FName AbilityVent(TEXT("Ability.Vent"));
	inline const FName AbilityMasterKey(TEXT("Ability.MasterKey"));
	inline const FName AbilityWireCutter(TEXT("Ability.WireCutter"));
	inline const FName AbilityDetect(TEXT("Ability.Detect"));
	inline const FName AbilityConceal(TEXT("Ability.Conceal"));
	inline const FName AbilityInvisible(TEXT("Ability.Invisible"));
	inline const FName AbilityUnlockDoor(TEXT("Ability.UnlockDoor"));
	inline const FName AbilityStealAmmo(TEXT("Ability.StealAmmo"));
	inline const FName AbilityLantern(TEXT("Ability.Lantern"));
	inline const FName AbilityFootprintVision(TEXT("Ability.FootprintVision"));
	inline const FName AbilityForceValve(TEXT("Ability.ForceValve"));

	// 상호작용 소품 (월드 3D — 전 클라)
	inline const FName GearBreak(TEXT("Gear.Break"));
	inline const FName GearRepair(TEXT("Gear.Repair"));
	inline const FName CoalDispense(TEXT("Coal.Dispense"));
	inline const FName SinkWashing(TEXT("Sink.Washing")); // 루프

	// 미니게임 (본인 로컬 2D)
	inline const FName QTEStart(TEXT("QTE.Start"));
	inline const FName QTESuccess(TEXT("QTE.Success"));
	inline const FName QTEFail(TEXT("QTE.Fail"));
	inline const FName ValveStart(TEXT("Valve.Start"));
	inline const FName ValveSuccess(TEXT("Valve.Success"));
	inline const FName ValveFail(TEXT("Valve.Fail"));

	// ── UI 사운드 DT (DT_UISounds) — MenuWidget 파생 위젯 / GODMainHUDWidget / 컨트롤러에 지정 ──
	inline const FName UIClick(TEXT("UI.Click"));
	inline const FName UIHover(TEXT("UI.Hover"));
	inline const FName UIMenuOpen(TEXT("UI.MenuOpen"));
	inline const FName UIMenuClose(TEXT("UI.MenuClose"));
	inline const FName UIChatReceive(TEXT("UI.ChatReceive"));
	inline const FName BGMMainMenu(TEXT("BGM.MainMenu")); // 루프
	inline const FName BGMInGame(TEXT("BGM.InGame"));     // 루프

	// ── 게임 전체 사운드 DT (DT_GameSounds) — GODGameState(BP) 에 지정. 열차/화로는 GameState 에서 읽어감 ──
	inline const FName GameStart(TEXT("Game.Start"));
	inline const FName GameVictory(TEXT("Game.Victory"));
	inline const FName GameDefeat(TEXT("Game.Defeat"));
	inline const FName GameGunsUnlocked(TEXT("Game.GunsUnlocked"));
	inline const FName WarningPressure(TEXT("Warning.Pressure"));
	inline const FName WarningFuelLow(TEXT("Warning.FuelLow"));
	inline const FName TrainRunning(TEXT("Train.Running")); // 루프
	inline const FName TrainExplosion(TEXT("Train.Explosion"));
	inline const FName TrainDerail(TEXT("Train.Derail"));
	inline const FName FurnaceBurning(TEXT("Furnace.Burning")); // 루프
}
