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

#include "KismetCompiler.h"
#include "KlawrBlueprint.h"
#include "KlawrBlueprintGeneratedClass.h"

namespace Klawr {

/**
 * Compiles Klawr Blueprints.
 */
class FKlawrBlueprintCompiler : public FKismetCompilerContext
{
public:
	FKlawrBlueprintCompiler(
		UKlawrBlueprint* Source, FCompilerResultsLog& OutResultsLog, 
		const FKismetCompilerOptions& CompilerOptions, TArray<UObject*>* ObjLoaded
	);

	virtual ~FKlawrBlueprintCompiler();

public: // FKismetCompilerContext interface
	virtual void Compile() override;
	virtual void KlawrCreateClassVariablesFromBlueprint(UKlawrBlueprintGeneratedClass* NewScripClass);
	virtual void KlawrCreateFunctionListFromBlueprint(UKlawrBlueprintGeneratedClass* NewScripClass);
	virtual void KlawrCreateFunction(FScriptFunction function, UKlawrBlueprintGeneratedClass* NewScripClass);
	virtual void CreateClassVariablesFromBlueprint() override;
	virtual void CreateFunctionList() override;

	UKlawrBlueprint* KlawrBlueprint() const { return Cast<UKlawrBlueprint>(Blueprint); }

protected: // FKismetCompilerContext interface
	virtual void SpawnNewClass(const FString& NewClassName) override;
	/** 
	 * Ensure that the given class is of the proper type (should be KlawrBlueprintGeneratedClass).
	 * @param GeneratedClass Class to check, if it's not of the correct type then GeneratedClass 
	 *                       will be set to nullptr.
	 */
	virtual void EnsureProperGeneratedClass(UClass*& GeneratedClass) override;
	/**
	 * Remove the properties and functions from the given class.
	 * @param ClassToClean A class to clean.
	 * @param OldCDO The old CDO of the class.
	 */
	virtual void CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& OldCDO) override;
	
	/** Perform final post-compilation setup, set flags, meta data, etc. */
	virtual void FinishCompilingClass(UClass* GeneratedClass) override;
	
	virtual bool ValidateGeneratedClass(UBlueprintGeneratedClass* GeneratedClass) override;

private:
	typedef FKismetCompilerContext Super;
	void CreateScriptContextProperty();
	
	TArray<FScriptField> ScriptDefinedFields;
	TArray<FScriptFunction> ScriptDefinedFunctions;
	UObjectProperty* ContextProperty;
};

} // namespace Klawr
