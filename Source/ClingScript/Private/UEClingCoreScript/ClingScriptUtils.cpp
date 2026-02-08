#include "ClingScriptUtils.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWindow.h"


TSharedRef<SWindow> FClingUIScriptUtils::CreateWindow(
	const TArray<TSharedPtr<SWidget>>& Widgets, const FWindowSettings& WindowSettings)
{
	// Helper lambda to add widgets to a box layout
	auto AddWidgetsToBox = [&Widgets]<typename TLayout>(TLayout BoxLayout)
	{
		for (const TSharedPtr<SWidget>& Widget : Widgets)
		{
			if (Widget.IsValid())
			{
				auto Slot = MoveTemp(
					BoxLayout->AddSlot()
					.Padding(5.0f));
				if constexpr (std::is_same_v<typename TLayout::ElementType,SVerticalBox>)					
					Slot.AutoHeight();
				if constexpr (std::is_same_v<typename TLayout::ElementType,SHorizontalBox>)
					Slot.AutoWidth();
				Slot[
					Widget.ToSharedRef()
				];
			}
		}
	};
	// Determine the layout type based on WindowSettings
	TSharedPtr<SWidget> LayoutContainer;
	if (WindowSettings.Layout == EWindowLayout::Horizontal)
	{
		TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
		AddWidgetsToBox(HorizontalBox);
		LayoutContainer = HorizontalBox;
	}
	else // Default to vertical layout
	{
		TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);
		AddWidgetsToBox(VerticalBox);
		LayoutContainer = VerticalBox;
	}
	
	// Create a window with the vertical layout
	TSharedRef<SWindow> RenameWindow = SNew(SWindow)
	.Title(FText::FromString(WindowSettings.Title))
	.ClientSize(FVector2D(WindowSettings.Size.X, WindowSettings.Size.Y))
	.SupportsMaximize(true)
	.SupportsMinimize(true)
	[
		LayoutContainer.ToSharedRef()
	];
	return RenameWindow;
}

void FClingUIScriptUtils::ShowWindow(const TSharedRef<SWindow>& Window, bool bShowModal)
{	
	if (bShowModal)
	{
		FSlateApplication::Get().AddModalWindow(Window, nullptr, true);	
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);	
	}	
}
