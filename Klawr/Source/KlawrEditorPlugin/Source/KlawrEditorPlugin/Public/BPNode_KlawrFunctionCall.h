#pragma once
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "KlawrBlueprintGeneratedClass.h"
#include "BPNode_KlawrFunctionCall.generated.h"


UCLASS(MinimalAPI)
class UBPNode_KlawrFunctionCall : public UK2Node
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual FText GetTooltipText() const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override;
	virtual void ReconstructNode() override;
	// End UEdGraphNode interface.
	
	// Begin UK2Node interface.
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;
	virtual FText GetMenuCategory() const override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	// End UK2Node interface.

	UEdGraphPin* GetThenPin() const;
private:
	FText NodeTooltip;
	void ChangePinType(UEdGraphPin* Pin);
	void ChangeNodeToMethod(FText inPar);
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FString PinDescription) const;
	bool IsParameterPin(UEdGraphPin* Pin);
	void UnlinkAndRemovePins();
	void AllocateFunctionParameterPins(UEdGraphPin* InputNodePin = NULL);
	UPROPERTY()
	TArray<FText> CSFunctionNames;
	UPROPERTY()
	FString RawFunctionName;
	UPROPERTY()
	UObject* dummyObject;
};