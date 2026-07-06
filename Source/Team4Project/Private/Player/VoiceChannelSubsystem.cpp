#include "Player/VoiceChannelSubsystem.h"
#include "Player/Component/VoipTalkerComponent.h"

void UVoiceChannelSubsystem::RegisterActiveTalker(UVoipTalkerComponent* Talker)
{
	if (IsValid(Talker))
	{
		ActiveTalkers.AddUnique(Talker);
	}
}

void UVoiceChannelSubsystem::UnregisterActiveTalker(UVoipTalkerComponent* Talker)
{
	ActiveTalkers.RemoveAll([Talker](const TWeakObjectPtr<UVoipTalkerComponent>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Talker;
	});
}

void UVoiceChannelSubsystem::RefreshVoicePolicies()
{
	for (int32 i = ActiveTalkers.Num() - 1; i >= 0; --i)
	{
		if (UVoipTalkerComponent* Talker = ActiveTalkers[i].Get())
		{
			Talker->ApplyVoicePolicy();
		}
		else
		{
			ActiveTalkers.RemoveAt(i);
		}
	}
}
