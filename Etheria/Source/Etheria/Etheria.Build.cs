// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Etheria : ModuleRules
{
	public Etheria(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"UMG", 
			"EnhancedInput", 
			"Niagara", 
			"AIModule", 
			"NavigationSystem", 
			"GameplayTasks",
			"GameplayTags",
			"GameplayStateTreeModule",
			"CableComponent",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			//"UMGEditor"
		});
	}
}