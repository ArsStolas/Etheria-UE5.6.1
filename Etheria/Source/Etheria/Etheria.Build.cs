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
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Etheria",
			"Etheria/Variant_Platforming",
			"Etheria/Variant_Platforming/Animation",
			"Etheria/Variant_Combat",
			"Etheria/Variant_Combat/AI",
			"Etheria/Variant_Combat/Animation",
			"Etheria/Variant_Combat/Gameplay",
			"Etheria/Variant_Combat/Interfaces",
			"Etheria/Variant_Combat/UI",
			"Etheria/Variant_SideScrolling",
			"Etheria/Variant_SideScrolling/AI",
			"Etheria/Variant_SideScrolling/Gameplay",
			"Etheria/Variant_SideScrolling/Interfaces",
			"Etheria/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
