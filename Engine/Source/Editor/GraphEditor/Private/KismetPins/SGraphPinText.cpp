// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphPinText.h"


void SGraphPinText::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SGraphPinText::GetDefaultValueWidget()
{
	return SNew(SBox)
		.MinDesiredWidth(18.0f)
		[
			SNew(SMultiLineEditableTextBox)
			.Style(FEditorStyle::Get(), "Graph.EditableTextBox")
			.Text(this, &SGraphPinText::GetTypeInValue)
			.SelectAllTextWhenFocused(true)
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			.IsReadOnly(this, &SGraphPinText::GetDefaultValueIsReadOnly)
			.OnTextCommitted(this, &SGraphPinText::SetTypeInValue)
			.ForegroundColor(FSlateColor::UseForeground())
			.ModiferKeyForNewLine(EModifierKey::Shift)
		];
}

FText SGraphPinText::GetTypeInValue() const
{
	return GraphPinObj->DefaultTextValue;
}

void SGraphPinText::SetTypeInValue(const FText& NewTypeInValue, ETextCommit::Type /*CommitInfo*/)
{
	// Update the localization string.
	GraphPinObj->GetSchema()->TrySetDefaultText(*GraphPinObj, NewTypeInValue);
}

bool SGraphPinText::GetDefaultValueIsReadOnly() const
{
	return GraphPinObj->bDefaultValueIsReadOnly;
}