#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Module for managing source code file access
 * Handles opening files in IDE or fallback to notepad
 */
class CLINGSOURCEACCESS_API FClingSourceAccessModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    /**
     * Open a file at a specific line in the configured IDE
     * Falls back to notepad if no IDE is configured
     * NOTE: This function does NOT check if file exists - use EnsureFileOpenInIDE for that
     * @param FilePath - Full path to the file
     * @param LineNumber - Line number to navigate to (1-based)
     * @return true if file was opened successfully
     */
    static bool OpenFileInIDE(const FString& FilePath, int32 LineNumber = 1);
    
    /**
     * Ensure a file is opened in IDE with proper error handling
     * This is the recommended API for file opening with validation
     * @param FilePath - Full path to the file (must exist)
     * @param LineNumber - Line number to navigate to (1-based)
     * @param bSuppressErrorUI - If true, won't broadcast open failed event
     * @return true if file exists and was opened
     */
    static bool EnsureFileOpenInIDE(const FString& FilePath, int32 LineNumber = 1, bool bSuppressErrorUI = false);
    
    /**
     * Open a file in system's default text editor (typically notepad on Windows)
     * @param FilePath - Full path to the file
     */
    static void OpenFileInNotepad(const FString& FilePath);
    
    /**
     * Check if source code access is properly configured
     * @return true if an IDE accessor is available
     */
    static bool IsIDEAccessAvailable();
};

/**
 * RAII helper to ensure a file is opened in IDE when scope exits
 * Automatically handles file existence checking and error reporting
 * Usage:
 *   FClingSourceFileOpener FileOpener;
 *   // Do work to generate file path and line number
 *   FileOpener.FilePath = "path/to/file.h";
 *   FileOpener.LineNumber = 42;
 *   // File will be opened automatically when FileOpener goes out of scope
 */
struct CLINGSOURCEACCESS_API FClingSourceFileOpener
{
	/** File path to open (set before destruction) */
	FString FilePath;
	
	/** Line number to navigate to (1-based) */
	int32 LineNumber = 1;
	
	/** Whether to skip opening (useful for conditional opening) */
	bool bSkipOpening = false;
	
	/** If true, won't show error UI when file doesn't exist */
	bool bSuppressErrorUI = false;
	
	/** Destructor opens the file if FilePath is set and file exists */
	~FClingSourceFileOpener();
};
