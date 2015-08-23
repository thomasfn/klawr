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
		auto Blueprint = CastChecked<UKlawrBlueprint>(Super::Blueprint);
		auto GeneratedClass = CastChecked<UKlawrBlueprintGeneratedClass>(InGeneratedClass);
		if (Blueprint && GeneratedClass)
		{
			GeneratedClass->ScriptDefinedType = Blueprint->ScriptDefinedType;
		}

		// allow classes generated from Blueprints of Klawr script components to be used in other 
		// Blueprints
		if (Super::Blueprint->ParentClass->IsChildOf(UKlawrScriptComponent::StaticClass()) &&
			(InGeneratedClass != Super::Blueprint->SkeletonGeneratedClass))
		{
			InGeneratedClass->SetMetaData(TEXT("BlueprintSpawnableComponent"), TEXT("true"));
		}

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
			UClass* InnerType = Field.Class;
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
					if (ScriptProperty != NULL)
					{
						ScriptProperty->SetMetaData(TEXT("Category"), *Blueprint->GetName());
						ScriptProperty->SetPropertyFlags(CPF_BlueprintVisible | CPF_Edit);
						
						NewScripClass->ScriptProperties.Add(ScriptProperty);
					}
				}
			}
		}

		CreateScriptContextProperty();
	}

	void FKlawrBlueprintCompiler::CreateScriptContextProperty()
	{
		// The only case we don't need a script context is if the script class derives form UScriptPluginComponent
		UClass* ContextClass = nullptr;
		if (Blueprint->ParentClass->IsChildOf(AActor::StaticClass()))
		{
			ContextClass = UKlawrScriptComponent::StaticClass();
		}
		/*
		else if (!Blueprint->ParentClass->IsChildOf(UKlawrPluginComponent::StaticClass()))
		{
		ContextClass = UScriptContext::StaticClass();
		}
		*/

		if (ContextClass)
		{
			FEdGraphPinType ScriptContextPinType(Schema->PC_Object, TEXT(""), ContextClass, false, false);
			ContextProperty = CastChecked<UObjectProperty>(CreateVariable(TEXT("Generated_ScriptContext"), ScriptContextPinType));
			ContextProperty->SetPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference);
		}
	}
} // namespace Klawr
