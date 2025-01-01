// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Reflection;
using System.Collections.Generic;
using System.IO;
using UnrealBuildBase;

public class ClingRuntime : ModuleRules
{
	public ClingRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// Let UBT export GlobalDefines of build
		var Field = Target.GetType().GetField("Inner",BindingFlags.Instance|BindingFlags.NonPublic);
		TargetRules TargetInner = (TargetRules) Field.GetValue(Target);
		TargetInner.ExportPublicHeader = "UECling/BuildGlobalDefines.h";
		
		// Create Bat of runt UBT JsonExport mode
		string JsonFile = Path.Combine(ModuleDirectory,"..","..","ModuleBuildInfos.json");
		string UBTPath = Path.Combine(Unreal.EngineDirectory.ToString(), "Binaries", "DotNET", "UnrealBuildTool", "UnrealBuildTool.exe");
		// we can not run UnrealBuildTool.Utils.RunLocalProcessAndReturnStdOut(UBTPath,..) here!
		// it will be called recursively!
		File.WriteAllText(Path.Combine(ModuleDirectory,"..","..","ExportModuleBuildInfos.bat"),
		Utils.FormatCommandLine(new List<string>(new string[]
		{
			Target.Name,Target.Platform.ToString(),Target.Configuration.ToString(),"-Mode=JsonExport","-Project="+Target.ProjectFile,"-NoMutex","-OutputFile="+JsonFile
		}))
        );
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ClingLibrary",
				"Projects",
				"Engine",
				"CoreUObject"
				// ... add other public dependencies that you statically link with here ...
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Engine",
				"DeveloperSettings",
				"Json",
				"JsonUtilities"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
