// Copyright 2025 Ivan Chandra. All Rights Reserved.
using EpicGames.Core;  // IMPORTANT - CONTAINS JsonObject 
using UnrealBuildTool;

public class OrionRPG : ModuleRules
{
    public OrionRPG(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bLegacyPublicIncludePaths = false;

        PublicIncludePaths.AddRange(
            new string[] {
            }
            );

        PrivateIncludePaths.AddRange(
            new string[] {
                "OrionRPG/Private",
            }
            );

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "CommonUI",
                "UMG",
                "CommonInput",
                "EnhancedInput",
                "InputCore",
                "DeveloperSettings",
                "LevelSequence",
                "MovieScene",
                "CinematicCamera",

            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "GameplayTags",
                "MovieSceneTracks",
            }
            );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
			}
            );

        // Add pre-processor macros for the OrionRPGFramework plugin based on enabled state (optional plugin)
        PublicDefinitions.Add("WITH_ORION_COMBAT_FRAMEWORK=0");
        if (JsonObject.TryRead(Target.ProjectFile, out var rawObject))
        {
            if (rawObject.TryGetObjectArrayField("Plugins", out var pluginObjects))
            {
                foreach (JsonObject pluginObject in pluginObjects)
                {
                    pluginObject.TryGetStringField("Name", out var pluginName);

                    pluginObject.TryGetBoolField("Enabled", out var pluginEnabled);

                    if (pluginName == "OrionCombatFramework" && pluginEnabled)
                    {
                        PrivateDependencyModuleNames.Add("OrionCombatFramework");
                        PublicDefinitions.Add("WITH_ORION_COMBAT_FRAMEWORK=1");
                        PublicDefinitions.Remove("WITH_ORION_COMBAT_FRAMEWORK=0");
                    }
                }
            }
        }

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.Add("EditorStyle");
        }
    }
}