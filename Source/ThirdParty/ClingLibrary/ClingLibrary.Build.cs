// Fill out your copyright notice in the Description page of Project Settings.

using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Extensions.Logging;
using UnrealBuildTool;

public class ClingLibrary : ModuleRules
{
	public ClingLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicSystemIncludePaths.Add("$(ModuleDir)/LLVM/include");
		
		List<string> Libs = new List<string>();
		Libs.AddRange(new string[]
		{
			"clangCppInterOp"
		});
		string ConfigurationType = "RelWithDebInfo";
		if (Target.Configuration == UnrealTargetConfiguration.Debug)
		{
			ConfigurationType = "Debug";
			PublicDefinitions.Add("USING_CPPINTEROP_DEBUG = 1");
		}
		else
		{
			ConfigurationType = "RelWithDebInfo";
			PublicDefinitions.Add("USING_CPPINTEROP_DEBUG = 0");
		}

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			foreach (var Lib in Libs)
			{
				string filePath = Path.Combine(ModuleDirectory, "LLVM", "lib", ConfigurationType, Lib + ".lib");
				PublicAdditionalLibraries.Add(filePath);
			}
			string LLVMBinDir = Path.Combine(ModuleDirectory, "LLVM", "bin", ConfigurationType);
			PrivateRuntimeLibraryPaths.Add(LLVMBinDir);
			foreach (var dll in Libs)
			{
				string dllPath = Path.Combine(LLVMBinDir, dll + ".dll");
				RuntimeDependencies.Add(dllPath, StagedFileType.NonUFS);
			}
			
			// PublicDefinitions.AddRange(new string[]
			// {
			// 	"LLVM_NO_DEAD_STRIP=1",
			// 	"CMAKE_CXX_STANDARD=17",
			// 	// "CMAKE_CXX_STANDARD_REQUIRED ON"
			// });
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
		// 	PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
		// 	RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/ClingLibrary/Mac/Release/libExampleLibrary.dylib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
		// 	string ExampleSoPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "ClingLibrary", "Linux", "x86_64-unknown-linux-gnu", "libExampleLibrary.so");
		// 	PublicAdditionalLibraries.Add(ExampleSoPath);
		// 	PublicDelayLoadDLLs.Add(ExampleSoPath);
		// 	RuntimeDependencies.Add(ExampleSoPath);
		}
	}
}
