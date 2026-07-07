// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Team4Project : ModuleRules
{
	public Team4Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput", 
			"AIModule", 
			"NavigationSystem", 
			"GameplayAbilities", 
			"GameplayTasks", 
			"GameplayTags",
			"PhysicsCore",
			"NetCore",
			"Niagara", 
			"Slate", 
			"SlateCore", 
			"UMG",
			"StructUtils",
			"AdvancedSessions",
			"OnlineSubsystem", 
			"OnlineSubsystemSteam", 
			"OnlineSubsystemUtils",
            "PhysicsCore"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		PublicIncludePaths.AddRange(new string[]
			{
				"Team4Project",
			}
		);
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
