// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Utility class for resolving C++ include paths within the Cling environment.
 */
class CLINGRUNTIME_API FClingIncludeUtils
{
public:
	/**
	 * Find the full path to an included file based on configured include paths.
	 * @param IncludePath The path as it appears in the #include statement (e.g., "CoreMinimal.h" or "Misc/Paths.h")
	 * @return The full path to the file if found, or an empty string otherwise.
	 */
	static FString FindIncludeFilePath(const FString& IncludePath);

private:
	/** Cache for found file paths to improve performance */
	static TMap<FString, FString> IncludePathCache;
};
