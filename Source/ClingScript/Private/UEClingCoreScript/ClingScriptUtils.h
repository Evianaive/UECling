#pragma once
#include "CoreMinimal.h"

class SWidget;
class SWindow;

class FClingUIScriptUtils
{
public:	

	// Layout direction for window widgets
	enum class EWindowLayout : uint8
	{
		Vertical,   // Top to bottom
		Horizontal  // Left to right
	};
	
	struct FWindowSettings
	{
		const FString& Title = TEXT("Cling Untitled Window");
		FVector2D Size = FVector2D(800, 400);
		EWindowLayout Layout = EWindowLayout::Vertical;		
	};
	static TSharedRef<SWindow> CreateWindow(
		const TArray<TSharedPtr<SWidget>>& Widgets, 
		const FWindowSettings& WindowSettings = FWindowSettings());
	
	static void ShowWindow(const TSharedRef<SWindow>& Window, bool bShowModal = false);
	
};
