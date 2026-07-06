// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MenuInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMenuInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class TEAM4PROJECT_API IMenuInterface
{
	GENERATED_BODY()

public:
	// Password가 비어있으면 공개 방
	virtual void Host(FString ServerName, FString Password) = 0;
	// 비밀번호 방이면 Password 필요 (클라 선검증 + 서버 PreLogin 최종 검증)
	virtual void Join(uint32 Index, FString Password) = 0;
	// 빠른 방찾기: 자리 있고 비밀번호 없는 방 중 최적(인원 많음 > 핑 낮음)을 자동 조인
	virtual void QuickJoin() = 0;
	virtual void LoadMainMenu() = 0;
	virtual void RefreshServerList() = 0;
};
