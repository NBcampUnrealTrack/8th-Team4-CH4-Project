#pragma once

#include "CoreMinimal.h"
#include "AnnouncementTypes.generated.h"

/**
 * 알림 방송의 심각도. WBP 에서 색상/애니메이션 분기용으로 쓴다.
 */
UENUM(BlueprintType)
enum class EAnnouncementType : uint8
{
	// 진행 상황 안내 (수리 완료, 목적지 임박 등)
	Info UMETA(DisplayName = "정보"),

	// 대응이 필요한 상황 (기어 파손, 압력 경고, 연료 부족)
	Warning UMETA(DisplayName = "경고"),

	// 되돌릴 수 없거나 즉시 패배로 이어지는 상황 (탈선, 연료 소진, 시간 임박)
	Critical UMETA(DisplayName = "치명")
};
