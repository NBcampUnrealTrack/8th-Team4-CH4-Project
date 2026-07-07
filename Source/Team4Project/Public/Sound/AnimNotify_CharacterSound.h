// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CharacterSound.generated.h"

/**
 * 애니메이션 타임라인용 사운드 노티파이.
 */
UCLASS(meta = (DisplayName = "Character Sound (DT)"))
class TEAM4PROJECT_API UAnimNotify_CharacterSound : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 캐릭터 사운드 DT 의 행 이름 (SoundRows 참조)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	FName RowName = TEXT("Footstep.Walk");

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 타임라인에 행 이름이 그대로 표시되게 한다 (노티파이 구분 편의).
	virtual FString GetNotifyName_Implementation() const override;
};
