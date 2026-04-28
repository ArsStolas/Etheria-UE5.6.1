// Copyright 2025 Ivan Chandra. All Rights Reserved.
using UnrealBuildTool;

public class Quest_System_Editor : ModuleRules
{
	public Quest_System_Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        bLegacyPublicIncludePaths = false;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] 
			{
			}
			);
		
		PrivateIncludePaths.AddRange(
			new string[] 
			{
                "Quest_System_Editor/Private",
                "Quest_System_Editor/Public",
			}
			);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "OrionRPG",
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
				"Kismet",
				"AIGraph",
				"EditorStyle",
                "AssetTools",
                "SlateCore",
                "Slate",
                "AssetDefinition",
            }
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
   {
				"AssetTools", 
				"GraphEditor", 
				"PropertyEditor", 
				"EditorStyle", 
				"Kismet", 
				"KismetWidgets", 
				"ApplicationCore", 
				"ToolMenus", 
				"Projects", 
				"InputCore", 
				"UnrealEd", 
				"ToolMenus", 
				"CoreUObject", 
				"Engine", 
				"Slate", 
				"SlateCore", 
				"AIGraph",
				"UMG",
   }
			);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}
