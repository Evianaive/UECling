// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text;
using EpicGames.Core;
using EpicGames.UHT.Utils;
using EpicGames.UHT.Tables;
using EpicGames.UHT.Exporters.CodeGen;
using EpicGames.UHT.Types;
using UnrealBuildBase;
using UnrealBuildTool;

namespace Plugins.ClingRuntime.UEClingIncludePathsPlugin;

[UnrealHeaderTool]
class GeneratedCodeModifier
{
	[UhtExporter(Name = "UEClingIncludePathsPlugin", Description = "Modify the thunk function of UFUNCTION", Options = UhtExporterOptions.Default, ModuleName="ClingRuntime")]
	private static void ScriptGeneratorExporter(IUhtExportFactory Factory)
	{
		// Factory.Session.LogInfo("This is a log44");
		new GeneratedCodeModifier(Factory).Modify();
	}
	
	GeneratedCodeModifier(IUhtExportFactory inFactory)
	{
		Factory = inFactory;
	}

	void Modify()
	{
		string UBTPath = Path.Combine(Unreal.EngineDirectory.ToString(), "Binaries", "DotNET", "UnrealBuildTool", "UnrealBuildTool.exe");
		if (Factory.Session.Manifest != null)
		{
			// string Target = Factory.Session.Manifest.TargetName;
			// string Platform = BuildHostPlatform.Current.Platform.ToString();
			// string? Project = Factory.Session.ProjectFile;

			// string JsonFile = Path.Combine(Factory.PluginModule!.IncludeBase,"..","ModuleBuildInfos.json");
			string BatFile = Path.Combine(Factory.PluginModule!.BaseDirectory,"../..", "ExportModuleBuildInfos.bat");
			
			string Result = UnrealBuildTool.Utils.RunLocalProcessAndReturnStdOut(
				UBTPath,
				// Utils.FormatCommandLine(new List<string>(new string[]
				// {
				// 	Target,Platform,"Development","-Mode=JsonExport","-Project="+Project,"-NoMutex","-OutputFile="+JsonFile
				// }))
				File.ReadAllText(BatFile)
				);
			// Factory.Session.LogInfo(Result);
			// BatFile = Path.Combine(Factory.PluginModule!.IncludeBase,"..", "ExportCompileCommands.bat");
			//
			// Result = UnrealBuildTool.Utils.RunLocalProcessAndReturnStdOut(
			// 	UBTPath,
			// 	// Utils.FormatCommandLine(new List<string>(new string[]
			// 	// {
			// 	// 	Target,Platform,"Development","-Mode=JsonExport","-Project="+Project,"-NoMutex","-OutputFile="+JsonFile
			// 	// }))
			// 	File.ReadAllText(BatFile)
			// );
			Factory.Session.LogInfo(Result);
		}
		
		// Create file of Generated Header Include Paths
		string GeneratedHeaderPathsFile = Path.Combine(Factory.PluginModule!.BaseDirectory,"../..", "GeneratedHeaderPaths.txt");
		List<string> GeneratedHeaderPaths = new List<string>();
		foreach (UhtModule package in Factory.Session.Modules)
		{
			GeneratedHeaderPaths.Add(package.Module.OutputDirectory);
		}
		File.WriteAllLines(GeneratedHeaderPathsFile,GeneratedHeaderPaths);
	}
	
	private IUhtExportFactory Factory;
}

