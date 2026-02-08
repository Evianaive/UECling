// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClingIncludeUtils.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ClingSetting.h"

TMap<FString, FString> FClingIncludeUtils::IncludePathCache;

FString FClingIncludeUtils::FindIncludeFilePath(const FString& IncludePath)
{
	// Check cache first
	if (FString* CachedPath = IncludePathCache.Find(IncludePath))
	{
		// Verify cached file still exists
		if (IFileManager::Get().FileExists(**CachedPath))
		{
			return *CachedPath;
		}
		else
		{
			// Remove invalid cache entry
			IncludePathCache.Remove(IncludePath);
		}
	}

	UClingSetting* ClingSetting = GetMutableDefault<UClingSetting>();
	if (!ClingSetting)
	{
		return FString();
	}

	FString FoundPath;

	// Search through all include paths
	ClingSetting->IterThroughIncludePaths([&](const FString& IncludePathDir)
	{
		if (!FoundPath.IsEmpty())
		{
			return; // Already found
		}

		// Normalize the include path directory
		FString NormalizedDir = IncludePathDir;
		FPaths::NormalizeDirectoryName(NormalizedDir);
		
		// Construct the full path
		FString TestPath = FPaths::Combine(NormalizedDir, IncludePath);
		TestPath = FPaths::ConvertRelativePathToFull(TestPath);
		FPaths::NormalizeFilename(TestPath);

		// Check if file exists
		if (IFileManager::Get().FileExists(*TestPath))
		{
			FoundPath = TestPath;
		}
	});

	// Cache the result if found
	if (!FoundPath.IsEmpty())
	{
		IncludePathCache.Add(IncludePath, FoundPath);
	}

	return FoundPath;
}
