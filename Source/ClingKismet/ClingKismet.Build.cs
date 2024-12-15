﻿using UnrealBuildTool;

public class ClingKismet : ModuleRules
{
    public ClingKismet(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UCling",
                "BlueprintGraph",
                "UnrealEd",
                "KismetCompiler"
            }
        );
    }
}