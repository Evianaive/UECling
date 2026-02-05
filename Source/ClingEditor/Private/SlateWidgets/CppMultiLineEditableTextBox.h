// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

/**
 * Extended multi-line editable text box with C++ syntax highlighting
 */
class CLINGEDITOR_API SCppMultiLineEditableTextBox : public SMultiLineEditableTextBox
{
public:
	using SMultiLineEditableTextBox::FArguments;
	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
};
