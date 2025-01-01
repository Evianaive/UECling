
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Text/RichTextLayoutMarshaller.h"
FText RichEditableText;
void MyScript4()
{	
	TSharedRef<FRichTextLayoutMarshaller> RichTextMarshaller = FRichTextLayoutMarshaller::Create(
			TArray<TSharedRef<ITextDecorator>>(), 
			&FAppStyle::Get()
			);
	TSharedRef<SWindow> RenameWindow = SNew(SWindow)
	// .Title(NSLOCTEXT("Script","FailedRenamesDialog", "Failed Renames"))
	.ClientSize(FVector2D(800,400))
	.SupportsMaximize(false)
	.SupportsMinimize(false)
	[
		SNew(SMultiLineEditableTextBox)
		.Font(FAppStyle::GetFontStyle("BoldFont"))
		.Text_Lambda([](){return RichEditableText;})
		.OnTextChanged_Lambda([](const FText& Text){RichEditableText = Text;})
		.OnTextCommitted_Lambda([](const FText& Text, ETextCommit::Type Type){RichEditableText = Text;})
		.Marshaller(RichTextMarshaller)
		.ClearTextSelectionOnFocusLoss(false)
		.AutoWrapText(true)
		.Margin(4)
		.LineHeightPercentage(1.1f)
	]
	;
	FSlateApplication::Get().AddWindow(RenameWindow);
}

	

