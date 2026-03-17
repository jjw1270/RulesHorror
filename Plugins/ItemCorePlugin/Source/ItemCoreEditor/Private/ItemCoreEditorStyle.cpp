// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemCoreEditorStyle.h"
#include "CommonUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"

TSharedPtr<FSlateStyleSet> FItemCoreEditorStyle::_StyleSet;

#define IMAGE_BRUSH(_relative_path, _size) \
	FSlateImageBrush(_StyleSet->RootToContentDir(_relative_path, TEXT(".png")), _size)

void FItemCoreEditorStyle::Initialize()
{
	if (IsValid(_StyleSet))
		return;

	_StyleSet = MakeShareable(new FSlateStyleSet("ItemCoreEditorStyle"));

	TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin("ItemCore");
	_StyleSet->SetContentRoot(plugin->GetBaseDir() / TEXT("Resources"));

	const FVector2D icon40(40.f, 40.f);
	_StyleSet->Set("ItemRegistry.Refresh", new IMAGE_BRUSH("ItemRegistryRefresh_40", icon40));

	FSlateStyleRegistry::RegisterSlateStyle(*_StyleSet.Get());
}

void FItemCoreEditorStyle::Shutdown()
{
	if (IsInvalid(_StyleSet))
		return;

	FSlateStyleRegistry::UnRegisterSlateStyle(*_StyleSet.Get());
	ensure(_StyleSet.IsUnique());
	_StyleSet.Reset();
}

const ISlateStyle& FItemCoreEditorStyle::Get()
{
	return *_StyleSet.Get();
}

FName FItemCoreEditorStyle::GetStyleSetName()
{
	static FName style_set_name(TEXT("ItemCoreEditorStyle"));
	return style_set_name;
}
