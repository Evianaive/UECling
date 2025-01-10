// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

class ISlateStyle;

class CLINGEDITOR_API FClingCodeEditorStyle
{
public:

	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get();

	static const FName& GetStyleSetName();

	static FTextBlockStyle NormalText;
private:
	/** Singleton instances of this style. */
	static TSharedPtr< class FSlateStyleSet > StyleSet;
};
