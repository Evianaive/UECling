using System;
using System.IO;
using UnrealBuildTool;

public class ClingScript : ModuleRules
{
    public ClingScript(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore"
            }
        );
        Action<string> ensureDirectoryExists = path =>
        {
            if (!Directory.Exists(path))
                if (path != null)
                    Directory.CreateDirectory(path);
                else
                    return;
            PublicIncludePaths.Add(path);
        };
        ensureDirectoryExists(Path.Combine(ModuleDirectory, "Private/FunctionLibrary"));
        ensureDirectoryExists(Path.Combine(ModuleDirectory, "Private/LambdaScript"));
    }
}