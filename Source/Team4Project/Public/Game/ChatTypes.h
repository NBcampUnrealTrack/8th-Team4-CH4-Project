#pragma once

#include "CoreMinimal.h"
#include "ChatTypes.generated.h"

USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SenderName;

	UPROPERTY(BlueprintReadOnly)
	FString Message;

	UPROPERTY(BlueprintReadOnly)
	float Timestamp = 0.f;
};