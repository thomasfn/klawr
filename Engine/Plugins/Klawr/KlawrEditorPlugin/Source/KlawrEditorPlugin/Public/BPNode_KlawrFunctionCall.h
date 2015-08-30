#pragma once
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "KlawrBlueprintGeneratedClass.h"
#include "BPNode_KlawrFunctionCall.generated.h"


UCLASS(MinimalAPI)
class UBPNode_KlawrFunctionCall : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinTypeChanged(UEdGraphPin* Pin) override;
	virtual FText GetTooltipText() const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override;
	// End UEdGraphNode interface.
	
	// Begin UK2Node interface.
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;
	virtual FText GetMenuCategory() const override;
	// End UK2Node interface.
	
private:
	FText NodeTooltip;
	void ChangePinType(UEdGraphPin* Pin);
	void ResetToWildcards(FText inPar);
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FText& PinDescription) const;
	void SetPinToolTip(UEdGraphPin& MutatablePin, const FString PinDescription) const;
	bool IsParameterPin(UEdGraphPin* Pin);
	void UnlinkAndRemovePins();
	TArray<FText> CSFunctionNames;
};