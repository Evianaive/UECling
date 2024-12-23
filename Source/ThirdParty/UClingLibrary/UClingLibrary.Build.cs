// Fill out your copyright notice in the Description page of Project Settings.

using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Extensions.Logging;
using UnrealBuildTool;

public class UClingLibrary : ModuleRules
{
	public UClingLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		// PublicSystemIncludePaths.Add("$(ModuleDir)/Public/");
		PublicSystemIncludePaths.Add("$(ModuleDir)/LLVM/include");
		// PublicSystemIncludePaths.Add("C:/Users/Evianaive/anaconda3/Library/include");
		// PublicAdditionalLibraries.Add("C:/Users/Evianaive/anaconda3/Library/lib/zlib.lib");
		// PublicAdditionalLibraries.Add("C:/Users/Evianaive/anaconda3/Library/lib/zstd.lib");
		
		// PublicSystemLibraryPaths.Add("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64");
		// PublicAdditionalLibraries.Add("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/Version.Lib");
		
		// PublicAdditionalLibraries.Add("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.41.34120/lib/x64/");
		// PublicSystemIncludePaths.Add("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.41.34120/include");
		// Ensure that the DLL is staged along with the executable
		List<string> Libs = new List<string>(); 
		Libs.AddRange(new string[]
		{
			// "obj.clangDriver",
			// "clangAnalysis","clangAnalysisFlowSensitive","clangAnalysisFlowSensitiveModels",
			// "clangAPINotes","clangARCMigrate","clangAST","clangASTMatchers","clangBasic",
			// "clangCodeGen","clangCrossTU","clangDependencyScanning","clangDirectoryWatcher",
			// "clangDriver","clangDynamicASTMatchers","clangEdit","clangExtractAPI","clangFormat",
			// "clangFrontend","clangFrontendTool","clangHandleCXX","clangHandleLLVM",
			// "clangIndex","clangIndexSerialization","clangInterpreter","clangLex",
			// "clangParse","clangRewrite","clangRewriteFrontend","clangSema",
			// "clangSerialization","clangStaticAnalyzerCheckers","clangStaticAnalyzerCore",
			// "clangStaticAnalyzerFrontend","clangSupport","clangTesting","clangTooling",
			// "clangToolingASTDiff","clangToolingCore","clangToolingInclusions",
			// "clangToolingInclusionsStdlib","clangToolingRefactoring","clangToolingSyntax",
			// "clangTransformer","clingInterpreter","clingMetaProcessor","clingUserInterface",
			// "clingUtils","DynamicLibraryLib","Kaleidoscope-Ch4","Kaleidoscope-Ch5","Kaleidoscope-Ch6",
			// "Kaleidoscope-Ch7","Kaleidoscope-Ch8","Kaleidoscope-Ch9",
			// "libclang","libcling",
			// "libclingJupyter",
			// "LLVM-C",
			// "LTO",
			// "Remarks",
			// "llvm_gtest","llvm_gtest_main","LLVMAggressiveInstCombine","LLVMAnalysis",
			// "LLVMAsmParser","LLVMAsmPrinter","LLVMBinaryFormat","LLVMBitReader","LLVMBitstreamReader",
			// "LLVMBitWriter",
			// "LLVMCFGuard","LLVMCFIVerify","LLVMCodeGen",
			// "LLVMCore","LLVMCoroutines","LLVMCoverage","LLVMDebugInfoCodeView",
			// "LLVMDebuginfod","LLVMDebugInfoDWARF","LLVMDebugInfoGSYM","LLVMDebugInfoLogicalView",
			// "LLVMDebugInfoMSF","LLVMDebugInfoPDB","LLVMDemangle","LLVMDiff","LLVMDlltoolDriver",
			// "LLVMDWARFLinker","LLVMDWARFLinkerParallel","LLVMDWP","LLVMExecutionEngine",
			// "LLVMExegesis","LLVMExegesisX86","LLVMExtensions","LLVMFileCheck",
			// "LLVMFrontendHLSL","LLVMFrontendOpenACC","LLVMFrontendOpenMP","LLVMFuzzerCLI",
			// "LLVMFuzzMutate","LLVMGlobalISel","LLVMInstCombine","LLVMInstrumentation",
			// "LLVMInterfaceStub","LLVMInterpreter","LLVMipo","LLVMIRPrinter",
			// "LLVMIRReader","LLVMJITLink","LLVMLibDriver","LLVMLineEditor","LLVMLinker",
			// "LLVMLTO","LLVMMC","LLVMMCA","LLVMMCDisassembler","LLVMMCJIT",
			// "LLVMMCParser","LLVMMIRParser","LLVMNVPTXCodeGen","LLVMNVPTXDesc",
			// "LLVMNVPTXInfo","LLVMObjCARCOpts","LLVMObjCopy","LLVMObject","LLVMObjectYAML",
			// "LLVMOption","LLVMOrcJIT","LLVMOrcShared","LLVMOrcTargetProcess","LLVMPasses",
			// "LLVMProfileData","LLVMRemarks","LLVMRuntimeDyld","LLVMScalarOpts",
			// "LLVMSelectionDAG","LLVMSupport","LLVMSymbolize","LLVMTableGen",
			// "LLVMTableGenGlobalISel","LLVMTarget","LLVMTargetParser","LLVMTestingAnnotations",
			// "LLVMTestingSupport","LLVMTextAPI","LLVMTransformUtils","LLVMVectorize",
			// "LLVMWindowsDriver","LLVMWindowsManifest","LLVMX86AsmParser","LLVMX86CodeGen",
			// "LLVMX86Desc","LLVMX86Disassembler","LLVMX86Info","LLVMX86TargetMCA",
			// "LLVMXRay",
			
			// "cling"
			"RelWithDebInfo/cling-demo"
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			foreach (var Lib in Libs)
			{
				string filePath = Path.Combine(ModuleDirectory, "LLVM", "lib", Lib + ".lib");
				PublicAdditionalLibraries.Add(filePath);
			}
			
			foreach (var dll in new string[]
			         {
				         "cling-demo.dll",
				         // "libclang.dll",
				         // "libcling.dll",
				         // "libclingJupyter.dll",
				         // "LLVM-C.dll",
				         // "LTO.dll",
				         // "Remarks.dll"
			         })
			{
				string BinariesDir = Path.Combine(PluginDirectory, "Binaries", Target.Platform.ToString());
				if (!Directory.Exists(BinariesDir))
				{
					Directory.CreateDirectory(BinariesDir);
				}
				Logger.LogInformation("Copying {dll}", dll);
				File.Copy(Path.Combine(ModuleDirectory,"LLVM","bin",dll),
					Path.Combine(BinariesDir,dll),
					true);
				RuntimeDependencies.Add(Path.Combine(BinariesDir, dll));
			}
			
			// Delay-load the DLL, so we can load it from the right place first
			// PublicDelayLoadDLLs.AddRange(new string[] {
			// 	"libclang.dll", 
			// 	"libcling", 
			// 	"libclingJupyter.dll",
			// 	"LLVM-C.dll", 
			// 	"LTO.dll", 
			// 	"Remarks.dll"
			// });

			// PrivateRuntimeLibraryPaths.Add(Path.Combine(ModuleDirectory,"LLVM","bin"));
			
			// PublicDefinitions.AddRange(new string[]
			// {
			// 	"LLVM_NO_DEAD_STRIP=1",
			// 	"CMAKE_CXX_STANDARD=17",
			// 	// "CMAKE_CXX_STANDARD_REQUIRED ON"
			// });
		}
		// else if (Target.Platform == UnrealTargetPlatform.Mac)
		// {
		// 	PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
		// 	RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/UClingLibrary/Mac/Release/libExampleLibrary.dylib");
		// }
		// else if (Target.Platform == UnrealTargetPlatform.Linux)
		// {
		// 	string ExampleSoPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "UClingLibrary", "Linux", "x86_64-unknown-linux-gnu", "libExampleLibrary.so");
		// 	PublicAdditionalLibraries.Add(ExampleSoPath);
		// 	PublicDelayLoadDLLs.Add(ExampleSoPath);
		// 	RuntimeDependencies.Add(ExampleSoPath);
		// }
	}
}
