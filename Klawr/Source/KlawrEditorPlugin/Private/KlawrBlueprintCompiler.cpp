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
#include "KlawrBlueprintCompiler.h"
#include "KlawrBlueprintGeneratedClass.h"
#include "KismetReinstanceUtilities.h"
#include "KlawrScriptComponent.h"
#include "IKlawrRuntimePlugin.h"
#include "Kismet2NameValidators.h"
#include "BlueprintEditorUtils.h"
#include "KlawrArgArray.h"

namespace Klawr {

	FKlawrBlueprintCompiler::FKlawrBlueprintCompiler(
		UKlawrBlueprint* Source, FCompilerResultsLog& OutResultsLog,
		const FKismetCompilerOptions& CompilerOptions, TArray<UObject*>* ObjLoaded
		) : Super(Source, OutResultsLog, CompilerOptions, ObjLoaded) 
	{
	}

	FKlawrBlueprintCompiler::~FKlawrBlueprintCompiler()
	{
	}

	void FKlawrBlueprintCompiler::Compile()
	{
		Super::Compile();
	}

	void FKlawrBlueprintCompiler::SpawnNewClass(const FString& NewClassName)
	{
		Super::NewClass = FindObject<UKlawrBlueprintGeneratedClass>(
			Super::Blueprint->GetOutermost(), *NewClassName
			);

		if (Super::NewClass)
		{
			FBlueprintCompileReinstancer::Create(Super::NewClass);
		}
		else
		{
			Super::NewClass = NewObject<UKlawrBlueprintGeneratedClass>(
				Super::Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional
				);
		}
	}

	void FKlawrBlueprintCompiler::EnsureProperGeneratedClass(UClass*& GeneratedClass)
	{
		auto ClassObject = Cast<UObject>(GeneratedClass);
		if (GeneratedClass && !ClassObject->IsA<UKlawrBlueprintGeneratedClass>())
		{
			FKismetCompilerUtilities::ConsignToOblivion(
				GeneratedClass, true
				);
			GeneratedClass = nullptr;
		}
	}

	void FKlawrBlueprintCompiler::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO)
	{
		Super::CleanAndSanitizeClass(ClassToClean, OldCDO);

		check(ClassToClean == Super::NewClass);
	}

	void FKlawrBlueprintCompiler::FinishCompilingClass(UClass* InGeneratedClass)
	{
		auto BP = CastChecked<UKlawrBlueprint>(Super::Blueprint);
		auto GeneratedClass = CastChecked<UKlawrBlueprintGeneratedClass>(InGeneratedClass);
		if (BP && GeneratedClass)
		{
			GeneratedClass->ScriptDefinedType = BP->ScriptDefinedType;
		}

		// allow classes generated from Blueprints of Klawr script components to be used in other 
		// Blueprints
		if (Super::Blueprint->ParentClass->IsChildOf(UKlawrScriptComponent::StaticClass()) &&
			(InGeneratedClass != Super::Blueprint->SkeletonGeneratedClass))
		{
			InGeneratedClass->SetMetaData(TEXT("BlueprintSpawnableComponent"), TEXT("true"));
		}
		FString dummy = InGeneratedClass->GetClass()->GetName();
		UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("InGeneratedClass: %s"), *dummy);

		Super::FinishCompilingClass(InGeneratedClass);
		UObject* CDO = InGeneratedClass->GetDefaultObject();

		for (TFieldIterator<UStrProperty> Property(InGeneratedClass); Property; ++Property)
		{
			if (Super::Blueprint->ParentClass->IsChildOf(UKlawrScriptComponent::StaticClass()) &&
				(InGeneratedClass != Super::Blueprint->SkeletonGeneratedClass))
			{
				Property->SetPropertyValue(Property->ContainerPtrToValuePtr<void>(CDO), FString(TEXT("")));
			}
		}

	}

	bool FKlawrBlueprintCompiler::ValidateGeneratedClass(UBlueprintGeneratedClass* GeneratedClass)
	{
		return Super::ValidateGeneratedClass(GeneratedClass)
			&& UKlawrBlueprint::ValidateGeneratedClass(GeneratedClass);
	}

	void FKlawrBlueprintCompiler::CreateClassVariablesFromBlueprint()
	{
		if (Super::Blueprint && Super::Blueprint->GeneratedClass)
		{
			UKlawrBlueprint* BP = CastChecked<UKlawrBlueprint>(Super::Blueprint);
			if (!BP->ScriptDefinedType.IsEmpty())
			{
				UKlawrBlueprintGeneratedClass* generatedClass = CastChecked<UKlawrBlueprintGeneratedClass>(BP->GeneratedClass);
				generatedClass->appDomainId = IKlawrRuntimePlugin::Get().GetObjectAppDomainID(generatedClass);
				KlawrCreateClassVariablesFromBlueprint(generatedClass);
			}
		}
		Super::CreateClassVariablesFromBlueprint();
	}

	void FKlawrBlueprintCompiler::KlawrCreateClassVariablesFromBlueprint(UKlawrBlueprintGeneratedClass* NewScripClass)
	{
		NewScripClass->ScriptProperties.Empty();
		NewScripClass->GetScriptDefinedFields(ScriptDefinedFields);

		for (auto& Field : ScriptDefinedFields)
		{
			UObject* InnerType = Field.Class;
			if (Field.Class->IsChildOf(UProperty::StaticClass()))
			{
				FString PinCategory;
				if (Field.Class->IsChildOf(UStrProperty::StaticClass()))
				{
					PinCategory = Schema->PC_String;
				}
				else if (Field.Class->IsChildOf(UFloatProperty::StaticClass()))
				{
					PinCategory = Schema->PC_Float;
				}
				else if (Field.Class->IsChildOf(UIntProperty::StaticClass()))
				{
					PinCategory = Schema->PC_Int;
				}
				else if (Field.Class->IsChildOf(UBoolProperty::StaticClass()))
				{
					PinCategory = Schema->PC_Boolean;
				}
				else if (Field.Class->IsChildOf(UByteProperty::StaticClass()))
				{
					PinCategory = Schema->PC_Byte;
					InnerType = Field.innerEnum;
				}
				else if (Field.Class->IsChildOf(UObjectProperty::StaticClass()))
				{
					PinCategory = Schema->PC_Object;
					// @todo: some scripting extensions (that are strongly typed) can handle this better

					InnerType = Field.innerClass;
				}
				if (!PinCategory.IsEmpty())
				{
					FEdGraphPinType ScriptPinType(PinCategory, TEXT(""), InnerType, false, false);
					UProperty* ScriptProperty = CreateVariable(Field.Name, ScriptPinType);

					bool hasCategoryMeta = false;
					for (auto meta : Field.metas)
					{
						hasCategoryMeta |= meta.Key == TEXT("Category");
						ScriptProperty->SetMetaData(*meta.Key, *meta.Value);
					}

					if (ScriptProperty != NULL)
					{
						// If no Category meta is provided in the UPROPERTY attribute, generate one out of the class name
						if (!hasCategoryMeta)
						{
							ScriptProperty->SetMetaData(TEXT("Category"), *Blueprint->GetName());
						}
						ScriptProperty->SetPropertyFlags(CPF_BlueprintVisible | CPF_Edit);

						if (NewScripClass->GetAdvancedDisplay(*(Field.Name.ToString())))
						{
							ScriptProperty->SetPropertyFlags(CPF_AdvancedDisplay);
						}

						if (NewScripClass->GetSaveGame(*(Field.Name.ToString())))
						{
							ScriptProperty->SetPropertyFlags(CPF_SaveGame);
						}

						NewScripClass->ScriptProperties.Add(ScriptProperty);
					}
				}
			}
		}
	}

	void FKlawrBlueprintCompiler::CreateScriptContextProperty()
	{
		// The only case we don't need a script context is if the script class derives form UScriptPluginComponent
		UClass* ContextClass = nullptr;
		if (Blueprint->ParentClass->IsChildOf(AActor::StaticClass()))
		{
			ContextClass = UKlawrScriptComponent::StaticClass();
		}
		else
		{
			ContextProperty = NULL;
		}

		if (ContextClass)
		{
			FEdGraphPinType ScriptContextPinType(Schema->PC_Object, TEXT(""), ContextClass, false, false);
			ContextProperty = CastChecked<UObjectProperty>(CreateVariable(TEXT("Generated_ScriptContext"), ScriptContextPinType));
			ContextProperty->SetPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference);
		}
	}

	void FKlawrBlueprintCompiler::CreateFunctionList()
	{
		if (Super::Blueprint && Super::Blueprint->GeneratedClass)
		{
			UKlawrBlueprint* BP = CastChecked<UKlawrBlueprint>(Super::Blueprint);
			if (!BP->ScriptDefinedType.IsEmpty())
			{
				UKlawrBlueprintGeneratedClass* generatedClass = CastChecked<UKlawrBlueprintGeneratedClass>(BP->GeneratedClass);
				generatedClass->appDomainId = IKlawrRuntimePlugin::Get().GetObjectAppDomainID(generatedClass);
				KlawrCreateFunctionListFromBlueprint(generatedClass);
			}
		}

		Super::CreateFunctionList();
	}
	
	void FKlawrBlueprintCompiler::KlawrCreateFunctionListFromBlueprint(UKlawrBlueprintGeneratedClass* newScriptClass)
	{
		newScriptClass->ScriptDefinedFunctions.Empty();
		newScriptClass->GetScriptDefinedFunctions(ScriptDefinedFunctions);

		for (auto function : ScriptDefinedFunctions)
		{
			KlawrCreateFunction(newScriptClass, function);
		}
	}

	void FKlawrBlueprintCompiler::KlawrCreateFunction(UKlawrBlueprintGeneratedClass* newScriptClass, FScriptFunction function)
	{
		UKlawrBlueprint* ScriptBP = KlawrBlueprint();
		const FString functionName = function.Name.ToString();

		UEdGraph* oldGraph = FindObject<UEdGraph>(ScriptBP, *FString::Printf(TEXT("%s_Graph"), *functionName));
		if (oldGraph != nullptr)
		{
			oldGraph->MarkPendingKill();
		}

		UEdGraph* scriptFunctionGraph = NewObject<UEdGraph>(ScriptBP, *FString::Printf(TEXT("%s_Graph"), *functionName));
		scriptFunctionGraph->Schema = UEdGraphSchema_K2::StaticClass();
		scriptFunctionGraph->SetFlags(RF_Transient);
		const UEdGraphSchema_K2* K2Schema = Cast<UEdGraphSchema_K2>(scriptFunctionGraph->GetSchema());

		FKismetFunctionContext* functionContext = CreateFunctionContext();
		functionContext->SourceGraph = scriptFunctionGraph;
		functionContext->bCreateDebugData = false;

		// Entry node
		UK2Node_FunctionEntry* entryNode = SpawnIntermediateNode<UK2Node_FunctionEntry>(nullptr, scriptFunctionGraph);
		entryNode->CustomGeneratedFunctionName = function.Name;
		entryNode->AllocateDefaultPins();
		UEdGraphPin* currentExec = K2Schema->FindExecutionPin(*entryNode, EGPD_Output);
		
		// Make an arg array
		UK2Node_CallFunction* makeArgArray = SpawnIntermediateNode<UK2Node_CallFunction>(nullptr, scriptFunctionGraph);
		makeArgArray->SetFromFunction(UKlawrArgArray::StaticClass()->FindFunctionByName(L"Create"));
		makeArgArray->AllocateDefaultPins();
		currentExec->MakeLinkTo(makeArgArray->GetExecPin());
		currentExec = makeArgArray->GetThenPin();

		// Iterate each parameter in the function
		int fIndex = 0;
		for (auto pair : function.Parameters)
		{
			FString PCType = TEXT("");
			UObject* innerObject = nullptr;
			TCHAR* funcName = nullptr;
			bool castRequired = false;

			// What parameter type is it?
			switch (pair.Value.Type)
			{
			case ParameterTypeTranslation::ParametertypeFloat:
				PCType = K2Schema->PC_Float;
				funcName = L"AddFloat";
				break;
			case ParameterTypeTranslation::ParametertypeInt:
				PCType = K2Schema->PC_Int;
				funcName = L"AddInt";
				break;
			case ParameterTypeTranslation::ParametertypeBool:
				PCType = K2Schema->PC_Boolean;
				funcName = L"AddBool";
				break;
			case ParameterTypeTranslation::ParametertypeString:
				PCType = K2Schema->PC_String;
				funcName = L"AddString";
				break;
			case ParameterTypeTranslation::ParametertypeEnum:
				PCType = K2Schema->PC_Byte;
				innerObject = pair.Value.innerEnum;
				funcName = L"AddInt";
				castRequired = true;
				break;
			case ParameterTypeTranslation::ParametertypeObject:
				PCType = K2Schema->PC_Object; 
				innerObject = pair.Value.innerClass;
				funcName = L"AddObject";
				break;
			}

			// Did we find a valid pin type?
			if (!PCType.IsEmpty())
			{
				// Create an output pin on the entry node for it
				FString paramName = pair.Key;
				UEdGraphPin *entryPin = entryNode->CreatePin(EGPD_Output, *PCType, TEXT(""), innerObject, false, false, paramName);

				// Add a cast if required
				if (castRequired)
				{

				}

				// Call argArray->Add*
				UK2Node_CallFunction* addArg = SpawnIntermediateNode<UK2Node_CallFunction>(nullptr, scriptFunctionGraph);
				addArg->SetFromFunction(UKlawrArgArray::StaticClass()->FindFunctionByName(funcName));
				addArg->AllocateDefaultPins();
				makeArgArray->GetReturnValuePin()->MakeLinkTo(addArg->FindPin(L"self"));
				UEdGraphPin* argPin = addArg->FindPin(L"value");
				entryPin->MakeLinkTo(argPin);
				addArg->NotifyPinConnectionListChanged(argPin);
				currentExec->MakeLinkTo(addArg->GetExecPin());
				currentExec = addArg->GetThenPin();
			}
			fIndex++;
		}

		// Exit node
		UK2Node_FunctionResult* exitNode = SpawnIntermediateNode<UK2Node_FunctionResult>(nullptr, scriptFunctionGraph);
		exitNode->AllocateDefaultPins();

		// Work out return type
		TCHAR* rawFuncName = nullptr;
		FString PCReturnType = TEXT("");
		UObject* returnInnerObject = nullptr;
		bool castRequired = false;
		switch (function.Result.Type)
		{
		case ParameterTypeTranslation::ParametertypeFloat:
			PCReturnType = K2Schema->PC_Float;
			rawFuncName = L"CallCSFunctionFloat";
			break;
		case ParameterTypeTranslation::ParametertypeInt:
			PCReturnType = K2Schema->PC_Int;
			rawFuncName = L"CallCSFunctionInt";
			break;
		case ParameterTypeTranslation::ParametertypeBool:
			PCReturnType = K2Schema->PC_Boolean;
			rawFuncName = L"CallCSFunctionBool";
			break;
		case ParameterTypeTranslation::ParametertypeString:
			PCReturnType = K2Schema->PC_String;
			rawFuncName = L"CallCSFunctionString";
			break;
		case ParameterTypeTranslation::ParametertypeEnum:
			PCReturnType = K2Schema->PC_Byte;
			returnInnerObject = function.Result.innerEnum;
			rawFuncName = L"CallCSFunctionInt";
			castRequired = true;
			break;
		case ParameterTypeTranslation::ParametertypeObject:
			PCReturnType = K2Schema->PC_Object;
			returnInnerObject = function.Result.innerClass;
			rawFuncName = L"CallCSFunctionObject";
			break;
		default:
			rawFuncName = L"CallCSFunctionVoid";
			break;
		}

		// Call function
		UK2Node_CallFunction* callFunction = SpawnIntermediateNode<UK2Node_CallFunction>(nullptr, scriptFunctionGraph);
		UFunction* callCSFunction = UKlawrScriptComponent::StaticClass()->FindFunctionByName(rawFuncName);
		if (callCSFunction == nullptr)
		{
			UE_LOG(LogKlawrEditorPlugin, Warning, TEXT("Function Name %s not found!"), rawFuncName);
			return;
		}
		callFunction->SetFromFunction(callCSFunction);
		callFunction->AllocateDefaultPins();
		callFunction->FindPinChecked(L"functionName")->DefaultValue = functionName;
		makeArgArray->GetReturnValuePin()->MakeLinkTo(callFunction->FindPinChecked(L"args", EGPD_Input));
		currentExec->MakeLinkTo(callFunction->GetExecPin());
		currentExec = callFunction->GetThenPin();


		// Did we find a valid return pin type?
		if (!PCReturnType.IsEmpty())
		{
			// Add a cast if required
			if (castRequired)
			{

			}

			// Create an input pin on the exit node for it
			UEdGraphPin *exitPin = exitNode->CreatePin(EGPD_Input, *PCReturnType, TEXT(""), returnInnerObject, false, false, L"result");
			callFunction->GetReturnValuePin()->MakeLinkTo(exitPin);
		}

		// Exit
		currentExec->MakeLinkTo(exitNode->GetExecPin());
	}
} // namespace Klawr
