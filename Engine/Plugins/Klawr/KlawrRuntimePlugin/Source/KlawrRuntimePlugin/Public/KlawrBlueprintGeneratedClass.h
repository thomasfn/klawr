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
#pragma once

#include "Engine/BlueprintGeneratedClass.h"
#include "KlawrBlueprintGeneratedClass.generated.h"


/**
* Script-defined field (variable or function)
*/
struct KLAWRRUNTIMEPLUGIN_API FScriptField
{
	/** Field name */
	FName Name;
	/** Field type */
	UClass* Class;

	UClass* innerClass;

	TMap<FString, FString> metas;

	FScriptField()
		: Class(NULL)
	{
	}
	FScriptField(FName InName, UClass* InClass)
		: Name(InName)
		, Class(InClass)
	{
	}
};


struct KLAWRRUNTIMEPLUGIN_API FScriptFunction
{
	FName Name;
	TMap<FString, FString> metas;
	TMap<FString, int> Parameters;
	TArray<UClass*> parameterClasses;
	int ResultType;

	FScriptFunction()
	{
	}

	FScriptFunction(FName InName)
		:Name(InName)
	{
	}
};

UCLASS()
class KLAWRRUNTIMEPLUGIN_API UKlawrBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_BODY()

public:
	
	UKlawrBlueprintGeneratedClass(const FObjectInitializer& objectInitializer);
	/** 
	 * The fully qualified name of the type defined in the script associated with an instance of
	 * this class.
	 */
	UPROPERTY()
	FString ScriptDefinedType;

	UPROPERTY()
	TArray<UProperty*> ScriptProperties;

	UPROPERTY()
	TArray<UFunction*> ScriptFunctions;

	TArray<FScriptFunction> ScriptDefinedFunctions;

	int appDomainId = 0;

public:

	void GetScriptDefinedFields(TArray<FScriptField>& OutFields);
	void GetScriptDefinedFunctions(TArray<FScriptFunction>& OutFunctions);

	bool GetAdvancedDisplay(const TCHAR* propertyName);
	bool GetSaveGame(const TCHAR* propertyName);
	/**
	 * Get the UKlawrBlueprintGeneratedClass from the inheritance hierarchy of the given class.
	 * @return UKlawrBlueprintGeneratedClass instance, or nullptr if the given class is not derived
	 *         from UKlawrBlueprintGeneratedClass
	 */
	FORCEINLINE static UKlawrBlueprintGeneratedClass* GetBlueprintGeneratedClass(UClass* Class)
	{
		UKlawrBlueprintGeneratedClass* GeneratedClass = nullptr;
		for (auto CurrentClass = Class; CurrentClass; CurrentClass = CurrentClass->GetSuperClass())
		{
			GeneratedClass = Cast<UKlawrBlueprintGeneratedClass>(CurrentClass);
			break;
		}
		return GeneratedClass;
	}
};
