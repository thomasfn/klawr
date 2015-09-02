// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "KlawrEditorPluginPrivatePCH.h"
#include "BPNode_KlawrFunctionCall.h"
#include "KlawrScriptComponent.h"

//BP
#include "KismetCompiler.h"
#include "BlueprintEditorUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2NameValidators.h"

//UnrealEd
#include "EditorCategoryUtils.h"
#include "Editor/GraphEditor/Public/DiffResults.h"
#include "EdGraph/EdGraphNodeUtils.h" // for FNodeTextCache
#include "EnumEditorUtils.h"
#include "Class.h"
//Undo/Redo
#include "ScopedTransaction.h"
#define LOCTEXT_NAMESPACE "BPNode_KlawrFunctionCall"

struct FGetConfigNodeName
{
	static const FString& GetInputObjectPinName()
	{
		static const FString SectionPinName(TEXT("InputComponent"));
		return SectionPinName;
	}

	static const FString& GetResultPinName()
	{
		static const FString ResultPinName(TEXT("Result"));
		return ResultPinName;
	}

	static const FString& GetFunctionNamePinName()
	{
		static const FString FunctionNamePinName(TEXT("FunctionName"));
		return FunctionNamePinName;
	}
};

UBPNode_KlawrFunctionCall::UBPNode_KlawrFunctionCall(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Call a C# function");
}

void UBPNode_KlawrFunctionCall::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Execute);
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);
	UEdGraphPin *SectionPin = CreatePin(EGPD_Input, K2Schema->PC_Object, TEXT(""), UKlawrScriptComponent::StaticClass(), false, false, FGetConfigNodeName::GetInputObjectPinName());
	SetPinToolTip(*SectionPin, LOCTEXT("KlawrScriptComponentPinTooltip", "Klawr script component to call function of"));

	UEdGraphPin *ResultPin = CreatePin(EGPD_Output, K2Schema->PC_Wildcard, TEXT(""), NULL, false, false, FGetConfigNodeName::GetResultPinName());
	SetPinToolTip(*ResultPin, LOCTEXT("ResultPinDescription", "Result value of C# function"));

	UEdGraphPin* functionNamePin = CreatePin(EGPD_Input, K2Schema->PC_String, TEXT(""), NULL, false, false, FGetConfigNodeName::GetFunctionNamePinName(), true);
	functionNamePin->bDefaultValueIsReadOnly = true;
	functionNamePin->bNotConnectable = true;

	AllocateFunctionParameterPins();
}

FText UBPNode_KlawrFunctionCall::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("KlawrNode_Title", "Klawr C# Call");
}

FText UBPNode_KlawrFunctionCall::GetTooltipText() const
{
	return NodeTooltip;
}

FName UBPNode_KlawrFunctionCall::GetPaletteIcon(FLinearColor& OutColor) const
{
	// The function icon seams the best choice!
	OutColor = GetNodeTitleColor();
	return TEXT("Kismet.AllClasses.FunctionIcon");

	//ArrayVariableIcon
	//VariableIcon

}

void UBPNode_KlawrFunctionCall::SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const
{
	MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToText(MutatablePin.PinType).ToString();

	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if (K2Schema != nullptr)
	{
		MutatablePin.PinToolTip += TEXT(" ");
		MutatablePin.PinToolTip += K2Schema->GetPinDisplayName(&MutatablePin).ToString();
	}

	MutatablePin.PinToolTip += FString(TEXT("\n")) + PinDescription.ToString();
}


void UBPNode_KlawrFunctionCall::SetPinToolTip(UEdGraphPin& MutatablePin, const FString PinDescription) const
{
	MutatablePin.PinToolTip = UEdGraphSchema_K2::TypeToText(MutatablePin.PinType).ToString();

	UEdGraphSchema_K2 const* const K2Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if (K2Schema != nullptr)
	{
		MutatablePin.PinToolTip += TEXT(" ");
		MutatablePin.PinToolTip += K2Schema->GetPinDisplayName(&MutatablePin).ToString();
		
	}

	MutatablePin.PinToolTip += FString(TEXT("\n")) + PinDescription;
}



void UBPNode_KlawrFunctionCall::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == FindPin(FGetConfigNodeName::GetInputObjectPinName()))
	{
		PinTypeChanged(Pin);
	}
}


void UBPNode_KlawrFunctionCall::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	/*
	const auto EaseFuncPin = GetEaseFuncPin();
	if (Pin == EaseFuncPin && ConditionalGenerateCustomPins())
	{
	GetGraph()->NotifyGraphChanged();
	}
	*/
}

void UBPNode_KlawrFunctionCall::PostReconstructNode()
{
	Super::PostReconstructNode();

}

//``
//~~~~~~~~~~~~~~~~~
// Allowed Types Here
//~~~~~~~~~~~~~~~~~
bool UBPNode_KlawrFunctionCall::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	/*
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Check the pin name and see if it matches on of our three base pins
	if (MyPin->PinName == FGetConfigNodeName::GetResultPinName())
	{
		const bool bConnectionOk =

			//! Basic types
			(OtherPin->PinType.PinCategory == K2Schema->PC_Float ||
			OtherPin->PinType.PinCategory == K2Schema->PC_Int ||
			OtherPin->PinType.PinCategory == K2Schema->PC_Boolean ||
			OtherPin->PinType.PinCategory == K2Schema->PC_String

			//! Struct Types
			|| (OtherPin->PinType.PinCategory == K2Schema->PC_Struct &&
			OtherPin->PinType.PinSubCategoryObject.IsValid() &&
			(OtherPin->PinType.PinSubCategoryObject.Get()->GetName() == TEXT("Vector") ||
			OtherPin->PinType.PinSubCategoryObject.Get()->GetName() == TEXT("Rotator") ||
			OtherPin->PinType.PinSubCategoryObject.Get()->GetName() == TEXT("LinearColor"))));
		if (!bConnectionOk)
		{
			OutReason = LOCTEXT("PinConnectionDisallowed", "This variable type cannot be loaded. Supported types are Bool, Int, Float, String, Vector, Rotator, and LinearColor.").ToString();
			return true;
		}
	}
	*/
	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UBPNode_KlawrFunctionCall::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)

	// Changed to UKlawrScriptComponent - Algorithman
	UClass* ActionKey = UKlawrScriptComponent::StaticClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UBPNode_KlawrFunctionCall::GetMenuCategory() const
{
	return LOCTEXT("KlawrCategory", "Klawr");
}

void UBPNode_KlawrFunctionCall::ChangePinType(UEdGraphPin* Pin)
{
	PinTypeChanged(Pin);
}

void UBPNode_KlawrFunctionCall::UnlinkAndRemovePins()
{
	// Pins 0 is input component pin, never to be removed
	for (int32 PinIndex = Pins.Num() - 1; PinIndex > 0; PinIndex--)
	{
		if (IsParameterPin(Pins[PinIndex]))
		{
			Pins[PinIndex]->BreakAllPinLinks();
			RemovePin(Pins[PinIndex]);
		}
	}
}


void UBPNode_KlawrFunctionCall::PinTypeChanged(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	bool bChanged = false;

	//! Is this the InputComponent Pin?
	if (Pin->PinName == FGetConfigNodeName::GetInputObjectPinName())
	{
		// Get pin refs
		UEdGraphPin* inputPin = FindPin(FGetConfigNodeName::GetResultPinName());

		// Propagate the type change or reset to parameter pins
		if (Pin->LinkedTo.Num() > 0)
		{
			UEdGraphPin* InstigatorPin = Pin->LinkedTo[0];

			bChanged = true;

			if (bChanged)
			{
				UKlawrBlueprintGeneratedClass* linkedToClass = CastChecked<UKlawrBlueprintGeneratedClass>(InstigatorPin->PinType.PinSubCategoryObject.Get());
				if (linkedToClass == NULL) 
				{
					UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("Cast went wrong"));
					return;
				}
				UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("Pin linked to %s"), *(linkedToClass->ScriptDefinedType));
				
				CSFunctionNames.Empty();
				for (auto scriptfunction : linkedToClass->ScriptDefinedFunctions)
				{
					CSFunctionNames.Add(FText::FromString(scriptfunction.Name.ToString()));
				}
			}
		}
		else
		{
			UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("Pin is no more linked"));
			/*
			//! Is Value pin unlinked?
			if (inputPin->GetDefaultAsString().IsEmpty() &&
				inputPin->LinkedTo.Num() == 0)
			{
				// Restore wild card pin
				inputPin->PinType.PinCategory = Schema->PC_Wildcard;
				inputPin->PinType.PinSubCategory = TEXT("");
				inputPin->PinType.PinSubCategoryObject = NULL;

				// Make sure the function name is nulled out
				bChanged = true;
			}
			*/
			UnlinkAndRemovePins();
			bChanged = true;

		}
	}
	if (Pin->PinName == FGetConfigNodeName::GetFunctionNamePinName())
	{
		UEdGraphPin* functionPin = FindPin(FGetConfigNodeName::GetInputObjectPinName());
		if (functionPin->LinkedTo.Num() > 0)
		{
			bChanged = true;
			UEdGraphPin* InstigatorPin = functionPin->LinkedTo[0];

			UKlawrBlueprintGeneratedClass* linkedToClass = CastChecked<UKlawrBlueprintGeneratedClass>(InstigatorPin->PinType.PinSubCategoryObject.Get());
			if (linkedToClass == NULL)
			{
				UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("Cast went wrong"));
				return;
			}
			UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("Pin linked to %s"), *(linkedToClass->ScriptDefinedType));

			UnlinkAndRemovePins();

			for (FScriptFunction scriptfunction : linkedToClass->ScriptDefinedFunctions)
			{
				if (scriptfunction.Name.ToString() == Pin->DefaultValue)
				{
					int fIndex = 0;
					for (auto key : scriptfunction.Parameters)
					{

						FString PCType = TEXT("");
						UClass* innerClass = NULL;

						switch (key.Value)
						{
						case 0: PCType = K2Schema->PC_Float; break;
						case 1: PCType = K2Schema->PC_Int; break;
						case 2: PCType = K2Schema->PC_Boolean; break;
						case 3: PCType = K2Schema->PC_String; break;
						case 4: PCType = K2Schema->PC_Object; innerClass = scriptfunction.parameterClasses[fIndex]; break;
						}
						if (!PCType.IsEmpty())
						{
							FString paramName = key.Key;
							UEdGraphPin *ResultPin = CreatePin(EGPD_Input, *PCType, TEXT(""), innerClass, false, false, key.Key);
							SetPinToolTip(*ResultPin, key.Key);
						}
						fIndex++;
					}
					break;
				}
			}
		}
	}
	// Pin connections and data changed in some way
	if (bChanged)
	{
		// Let the graph know to refresh
		GetGraph()->NotifyGraphChanged();

		UBlueprint* Blueprint = GetBlueprint();
		if (!Blueprint->bBeingCompiled)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			Blueprint->BroadcastChanged();
		}
	}

	Super::PinTypeChanged(Pin);
}


void UBPNode_KlawrFunctionCall::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	/**
	At the end of this, the BPNode_KlawrFunctionCall will not be a part of the Blueprint, it merely handles connecting
	the other nodes into the Blueprint.

	Care must be taken that the result pin from the visible node is transferred to the result pin of the internal function
	-Rama
	*/

	UK2Node_MakeArray* arrayFloat = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	arrayFloat->AllocateDefaultPins();
	UK2Node_MakeArray* arrayInt = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	arrayInt->AllocateDefaultPins();
	UK2Node_MakeArray* arrayBool = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	arrayBool->AllocateDefaultPins();
	UK2Node_MakeArray* arrayString = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	arrayString->AllocateDefaultPins();
	UK2Node_MakeArray* arrayObject = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	arrayObject->AllocateDefaultPins();
	
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* execStart = GetExecPin();

	TArray<UEdGraphPin*> parameterPins;
	for (UEdGraphPin* pin : Pins)
	{
		if (IsParameterPin(pin))
		{
			if (pin->PinType.PinCategory == K2Schema->PC_Float)
			{
				arrayFloat->AddInputPin();
				UEdGraphPin* newPin = arrayFloat->Pins.Last();
				CompilerContext.MovePinLinksToIntermediate(*pin,*newPin);
			}
			else if (pin->PinType.PinCategory == K2Schema->PC_Int)
			{
				arrayInt->AddInputPin();
				UEdGraphPin* newPin = arrayInt->Pins.Last();
				CompilerContext.MovePinLinksToIntermediate(*pin, *newPin);
			}
			else if (pin->PinType.PinCategory == K2Schema->PC_Boolean)
			{
				arrayBool->AddInputPin();
				UEdGraphPin* newPin = arrayBool->Pins.Last();
				CompilerContext.MovePinLinksToIntermediate(*pin, *newPin);
			}
			else if (pin->PinType.PinCategory == K2Schema->PC_String)
			{
				arrayString->AddInputPin();
				UEdGraphPin* newPin = arrayString->Pins.Last();
				CompilerContext.MovePinLinksToIntermediate(*pin, *newPin);
			}
			else if (pin->PinType.PinCategory == K2Schema->PC_Object)
			{
				arrayObject->AddInputPin();
				UEdGraphPin* newPin = arrayObject->Pins.Last();

				/*
				// Create the cast to UObject here
				UK2Node_DynamicCast* castNode = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
				castNode->TargetType = UObject::StaticClass();
				castNode->AllocateDefaultPins();
				castNode->GetExecPin()->MakeLinkTo(execStart);
				execStart = castNode->FindPin(K2Schema->PN_Then);
				

				UEdGraphPin* castSource = castNode->GetCastSourcePin();
				CompilerContext.MovePinLinksToIntermediate(*pin, *castSource);
				castNode->GetCastResultPin()->MakeLinkTo(newPin);
				*/
				CompilerContext.MovePinLinksToIntermediate(*pin, *newPin);

			}

		}
	}
	execStart->MakeLinkTo(FindPin(K2Schema->PN_Then));
	 
	
	//~~~ 
	/*
	// The call function does all the real work, each child class implementing easing for a given type provides
	// the name of the desired function
	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

	CallFunction->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	//~~~
	
	//!
	// Move the input pins to the function
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FGetConfigNodeName::GetInputObjectPinName()), *CallFunction->FindPin(FGetConfigNodeName::GetInputObjectPinName()));
	//!

	//~~~
	
	//! Connect the return pin of the visible node to the return pin of the internal function return value!
	CompilerContext.MovePinLinksToIntermediate(*FindPin(FGetConfigNodeName::GetResultPinName()), *CallFunction->GetReturnValuePin());
	*/
	// Cleanup links to ourselves and we are done!
	BreakAllNodeLinks();
}

void UBPNode_KlawrFunctionCall::ResetToWildcards(FText inPar)
{
	FScopedTransaction Transaction(LOCTEXT("ResetToDefaultsTx", "ResetToDefaults"));
	Modify();

	// Get pin refs
	UEdGraphPin* FunctionNamePin = FindPin(FGetConfigNodeName::GetFunctionNamePinName());

	// Set all to defaults and break links
	FunctionNamePin->DefaultValue = *(inPar.ToString());
	FunctionNamePin->BreakAllPinLinks();

	// Do the rest of the work, we will not recompile because the wildcard pins will prevent it
	PinTypeChanged(FunctionNamePin);
}

void UBPNode_KlawrFunctionCall::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	Super::GetContextMenuActions(Context);

	if (!Context.bIsDebugging)
	{
		// if (Context.Pin == FindPin(FGetConfigNodeName::GetFunctionNamePinName()))
		{
			Context.MenuBuilder->BeginSection("BPNode_KlawrFunctionCall", LOCTEXT("ContextMenuHeader", "Choose Function"));
			{
				for (auto fName : CSFunctionNames)
				{
					Context.MenuBuilder->AddMenuEntry(fName, fName, FSlateIcon(), FUIAction(FExecuteAction::CreateUObject(this, &UBPNode_KlawrFunctionCall::ResetToWildcards, fName)));
				}
			}
			Context.MenuBuilder->EndSection();
		}
	}
}


bool UBPNode_KlawrFunctionCall::IsParameterPin(UEdGraphPin* Pin)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	return ((Pin->PinName != FGetConfigNodeName::GetInputObjectPinName()) && (Pin->PinName != FGetConfigNodeName::GetResultPinName()) && (Pin->PinName != FGetConfigNodeName::GetFunctionNamePinName())
		&& (Pin->PinName != K2Schema->PN_Execute) && (Pin->PinName!=K2Schema->PN_Then));
}

void UBPNode_KlawrFunctionCall::AllocateFunctionParameterPins(UEdGraphPin* InputNodePin)
{
	UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("------------------------------------------->"));
	if (!InputNodePin)
	{
		PinTypeChanged(FindPin(FGetConfigNodeName::GetFunctionNamePinName()));
	}
	else
	{
		PinTypeChanged(InputNodePin);
	}
}

void UBPNode_KlawrFunctionCall::ReconstructNode() 
{
	Modify();

	UBlueprint* Blueprint = GetBlueprint();

	// Break any links to 'orphan' pins
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		TArray<class UEdGraphPin*> LinkedToCopy = Pin->LinkedTo;
		for (int32 LinkIdx = 0; LinkIdx < LinkedToCopy.Num(); LinkIdx++)
		{
			UEdGraphPin* OtherPin = LinkedToCopy[LinkIdx];
			// If we are linked to a pin that its owner doesn't know about, break that link
			if ((OtherPin == NULL) || !OtherPin->GetOwningNodeUnchecked() || !OtherPin->GetOwningNode()->Pins.Contains(OtherPin))
			{
				Pin->LinkedTo.Remove(OtherPin);
			}
		}
	}

	// Move the existing pins to a saved array
	TArray<UEdGraphPin*> OldPins(Pins);

	Pins.Empty();

	// Recreate the new pins
	ReallocatePinsDuringReconstruction(OldPins);

	bool bDestroyOldPins = true;

	TArray<UEdGraphPin*> OldPinsNonParameter;
	TArray<UEdGraphPin*> OldPinsParameter;

	TArray<UEdGraphPin*> NewPinsNonParameter;
	TArray<UEdGraphPin*> NewPinsParameter;

	for (auto pin : OldPins)
	{
		if (IsParameterPin(pin))
		{
			OldPinsParameter.Add(pin);
		}
		else
		{
			OldPinsNonParameter.Add(pin);
		}
	}

	for (auto pin : Pins)
	{
		if (IsParameterPin(pin))
		{
			NewPinsParameter.Add(pin);
		}
		else
		{
			NewPinsNonParameter.Add(pin);
		}
	}

	RewireOldPinsToNewPins(OldPinsNonParameter, NewPinsNonParameter);
	PinTypeChanged(FindPin(FGetConfigNodeName::GetFunctionNamePinName()));
	RewireOldPinsToNewPins(OldPinsParameter, Pins);


	if (bDestroyOldPins)
	{
		DestroyPinList(OldPins);
	}

	// Let subclasses do any additional work
	PostReconstructNode();

	GetGraph()->NotifyGraphChanged();

}

void UBPNode_KlawrFunctionCall::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	AllocateDefaultPins();

	for (auto OldPin : OldPins)
	{
		if (OldPin->ParentPin)
		{
			// find the new pin that corresponds to parent, and split it if it isn't already split
			for (auto NewPin : Pins)
			{
				if (FCString::Stricmp(*(NewPin->PinName), *(OldPin->ParentPin->PinName)) == 0)
				{
					// Make sure we're not dealing with a menu node
					UEdGraph* OuterGraph = GetGraph();
					if (OuterGraph && OuterGraph->Schema && NewPin->SubPins.Num() == 0)
					{
						NewPin->PinType = OldPin->ParentPin->PinType;
						GetSchema()->SplitPin(NewPin);
					}
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
