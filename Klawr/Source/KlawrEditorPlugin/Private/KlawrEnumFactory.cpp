//-------------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2014 Vadim Macagon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-------------------------------------------------------------------------------

#include "KlawrEditorPluginPrivatePCH.h"
#include "KlawrEnumFactory.h"
#include "KlawrScriptEnum.h"
#include "KlawrScriptsReloader.h"
#include "IKlawrRuntimePlugin.h"
#include "Widgets/SScriptTypeSelectionDialog.h"

#define LOCTEXT_NAMESPACE "KlawrEditorPlugin.UKlawrEnumFactory"

UKlawrEnumFactory::UKlawrEnumFactory(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	Super::SupportedClass = UKlawrScriptEnum::StaticClass();
	Super::bCreateNew = true;
	Super::bEditAfterNew = false;
	Super::bEditorImport = false;
	Super::bText = false;
	
	Super::Formats.Add(TEXT("cs;C# Source"));
}

bool UKlawrEnumFactory::DoesSupportClass(UClass* Class)
{
	return Class == UKlawrScriptEnum::StaticClass();
}

UObject* UKlawrEnumFactory::FactoryCreateNew(
	UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context,
	FFeedbackContext* Warn
)
{
	FString scriptType = Klawr::SScriptTypeSelectionDialog::SelectScriptType(
		LOCTEXT("NewEnumDialogTitle", "New Klawr Enum"), InName.ToString(), Klawr::ScriptType::ScriptEnum
	);

	if (scriptType.IsEmpty())
	{
		return nullptr;
	}

	/*auto newBlueprint = CastChecked<UKlawrScriptEnum>(
		FKismetEditorUtilities::CreateBlueprint(
			UKlawrScriptComponent::StaticClass(), InParent, InName, BPTYPE_Normal, 
			UKlawrBlueprint::StaticClass(),	UKlawrBlueprintGeneratedClass::StaticClass(), 
			"UKlawrBlueprintFactory"
		)
	);*/

	auto newEnum = NewObject<UKlawrScriptEnum>(InParent, InName, RF_Public | RF_Standalone | RF_Transactional | RF_LoadCompleted);
	newEnum->ScriptDefinedType = scriptType;
	TArray<TPair<FName, uint8>> emptyNames;
	newEnum->SetEnums(emptyNames, UEnum::ECppForm::Namespaced);
	newEnum->SetMetaData(TEXT("BlueprintType"), TEXT("true"));

	FEditorDelegates::OnAssetPostImport.Broadcast(this, newEnum);

	newEnum->RebuildFromType();

	// Disable and re-enable Scripts Reloader 
	// Fixes issue when you add a blueprint for the first time
	
	Klawr::FScriptsReloader::Get().Disable();
	Klawr::FScriptsReloader::Get().Enable();
	return newEnum;
}

#undef LOCTEXT_NAMESPACE
