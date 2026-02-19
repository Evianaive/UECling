#pragma once
#include "CoreMinimal.h"

class SWidget;
class SWindow;

// Todo Protect from windows temporarily, Fix it in PCH
#ifdef CreateWindow
#undef CreateWindow;
#endif

struct FWindowSettings
{
	// Layout direction for window widgets
	enum class EWindowLayout : uint8
	{
		Vertical,   // Top to bottom
		Horizontal  // Left to right
	};
	
	FString Title = GetDefaultTitle();
	FVector2D Size = FVector2D(800, 400);
	EWindowLayout Layout = EWindowLayout::Vertical;
	static FString GetDefaultTitle(){ return TEXT("Cling Untitled Window"); };
};

class CLINGSCRIPT_API FClingUIScriptUtils
{
public:	
	static TSharedRef<SWindow> CreateWindow(
		const TArray<TSharedPtr<SWidget>>& Widgets, 
		const FWindowSettings& WindowSettings = FWindowSettings());
	
	static void ShowWindow(const TSharedRef<SWindow>& Window, bool bShowModal = false);
	
};
