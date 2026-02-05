#include "ClingSourceAccess.h"
#include "ISourceCodeAccessModule.h"
#include "ISourceCodeAccessor.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FClingSourceAccessModule"

void FClingSourceAccessModule::StartupModule()
{
    
}

void FClingSourceAccessModule::ShutdownModule()
{
    
}

bool FClingSourceAccessModule::OpenFileInIDE(const FString& FilePath, int32 LineNumber)
{
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ClingSourceAccess: Cannot open empty file path"));
		return false;
	}
	
	// Note: This function does NOT check file existence
	// Use EnsureFileOpenInIDE if you need existence validation
	
	// Try to use IDE first
	if (IsIDEAccessAvailable())
	{
		ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
		ISourceCodeAccessor& SourceCodeAccessor = SourceCodeAccessModule.GetAccessor();
		
		UE_LOG(LogTemp, Log, TEXT("ClingSourceAccess: Opening file in IDE: %s at line %d"), *FilePath, LineNumber);
		bool bSuccess = SourceCodeAccessor.OpenFileAtLine(FilePath, LineNumber);
		
		if (bSuccess)
		{
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ClingSourceAccess: IDE failed to open file, falling back to notepad"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("ClingSourceAccess: No IDE configured, using notepad"));
	}
	
	// Fallback to notepad
	OpenFileInNotepad(FilePath);
	return true;
}

bool FClingSourceAccessModule::EnsureFileOpenInIDE(const FString& FilePath, int32 LineNumber, bool bSuppressErrorUI)
{
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ClingSourceAccess: Cannot ensure open - empty file path"));
		return false;
	}
	
	// Check if file exists - this is the "Ensure" part
	if (!FPaths::FileExists(FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("ClingSourceAccess: Cannot ensure open - file does not exist: %s"), *FilePath);
		
		// Broadcast file open failed event unless suppressed
		if (!bSuppressErrorUI && FModuleManager::Get().IsModuleLoaded("SourceCodeAccess"))
		{
			ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
			SourceCodeAccessModule.OnOpenFileFailed().Broadcast(FilePath);
		}
		
		return false;
	}
	
	// File exists, open it
	return OpenFileInIDE(FilePath, LineNumber);
}

void FClingSourceAccessModule::OpenFileInNotepad(const FString& FilePath)
{
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ClingSourceAccess: Cannot open empty file path in notepad"));
		return;
	}
	
	// Normalize the file path
	FString NormalizedPath = FilePath;
	FPaths::NormalizeFilename(NormalizedPath);
	
	// Open with default editor (typically notepad on Windows)
#if PLATFORM_WINDOWS
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*NormalizedPath, nullptr, ELaunchVerb::Edit);
	UE_LOG(LogTemp, Log, TEXT("ClingSourceAccess: Opened file in notepad: %s"), *NormalizedPath);
#else
	// On other platforms, try to open with default editor
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*NormalizedPath);
	UE_LOG(LogTemp, Log, TEXT("ClingSourceAccess: Opened file in default editor: %s"), *NormalizedPath);
#endif
}

bool FClingSourceAccessModule::IsIDEAccessAvailable()
{
	// Check if SourceCodeAccess module is loaded
	if (!FModuleManager::Get().IsModuleLoaded("SourceCodeAccess"))
	{
		// Try to load it
		if (!FModuleManager::Get().LoadModule("SourceCodeAccess"))
		{
			return false;
		}
	}
	
	ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
	ISourceCodeAccessor& SourceCodeAccessor = SourceCodeAccessModule.GetAccessor();
	
	// Check if accessor can open files
	// The default "None" accessor name typically means no IDE is configured
	FName AccessorName = SourceCodeAccessor.GetFName();
	if (AccessorName == NAME_None || AccessorName.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase))
	{
		return false;
	}
	
	return true;
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClingSourceAccessModule, ClingSourceAccess)

// ============================================================================
// FClingSourceFileOpener Implementation
// ============================================================================

FClingSourceFileOpener::~FClingSourceFileOpener()
{
	if (bSkipOpening || FilePath.IsEmpty())
	{
		return;
	}
	
	// Use EnsureFileOpenInIDE which handles file existence check internally
	FClingSourceAccessModule::EnsureFileOpenInIDE(FilePath, LineNumber, bSuppressErrorUI);
}