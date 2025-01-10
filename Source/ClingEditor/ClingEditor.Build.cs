using UnrealBuildTool;

public class ClingEditor : ModuleRules
{
    public ClingEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "CodeEditor"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "ClingRuntime",
                "UnrealEd",
                "ClingLibrary",
                "DeveloperSettings"
            }
        );
    }
}