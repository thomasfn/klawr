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

#include "KlawrRuntimePluginPrivatePCH.h"
#include "KlawrBlueprintGeneratedClass.h"


UKlawrBlueprintGeneratedClass::UKlawrBlueprintGeneratedClass(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	// Create the CDO as soon as possible (needed for issue with Asset Registry)
	auto dummy = this->StaticClass()->GetDefaultObject(true);
}

bool UKlawrBlueprintGeneratedClass::GetAdvancedDisplay(const TCHAR* propertyName)
{
	if (ScriptDefinedType.IsEmpty())
	{
		return false;
	}
	return Klawr::IClrHost::Get()->GetScriptComponentPropertyIsAdvancedDisplay(appDomainId, *ScriptDefinedType, propertyName);
}

bool UKlawrBlueprintGeneratedClass::GetSaveGame(const TCHAR* propertyName)
{
	if (ScriptDefinedType.IsEmpty())
	{
		return false;
	}
	return Klawr::IClrHost::Get()->GetScriptComponentPropertyIsSaveGame(appDomainId, *ScriptDefinedType, propertyName);
}

void UKlawrBlueprintGeneratedClass::GetScriptDefinedFields(TArray<FScriptField>& OutFields)
{
	OutFields.Empty();
	if (ScriptDefinedType.IsEmpty())
	{
		return;
	}

	// Read properties from generated class
	std::vector<Klawr::tstring> propertyNames;
	// We are most certainly in the editor atm
	Klawr::IClrHost::Get()->GetScriptComponentPropertyNames(appDomainId, *ScriptDefinedType, propertyNames);
	
	for (auto& propertyName : propertyNames)
	{
		int propertyType = Klawr::IClrHost::Get()->GetScriptComponentPropertyType(appDomainId, *ScriptDefinedType, propertyName.c_str());
		FScriptField propertyInfo;
		switch (propertyType)
		{
		case 0:
			propertyInfo.Class = UFloatProperty::StaticClass();
			break;
		case 1:
			propertyInfo.Class = UIntProperty::StaticClass();
			break;
		case 2:
			propertyInfo.Class = UBoolProperty::StaticClass();
			break;
		case 3:
			propertyInfo.Class = UStrProperty::StaticClass();
			break;
		case 4:
			propertyInfo.Class = UObjectProperty::StaticClass();

			const TCHAR* propertyClassName = Klawr::IClrHost::Get()->GetScriptComponentPropertyClassType(appDomainId, *ScriptDefinedType, propertyName.c_str());
			propertyInfo.innerClass = FindObject<UClass>(ANY_PACKAGE, propertyClassName);
			break;
		}

		if (propertyInfo.Class)
		{
			propertyInfo.Name = FName(propertyName.c_str());
			std::vector<Klawr::tstring> metaDatas;
			Klawr::IClrHost::Get()->GetScriptComponentPropertyMetadata(appDomainId, *ScriptDefinedType, propertyName.c_str(), metaDatas);
			bool keyValue = false;
			for (int i = 0; i < metaDatas.size(); i += 2)
			{
				propertyInfo.metas.Add(FString(metaDatas[i].c_str()), FString(metaDatas[i + 1].c_str()));
			}

			OutFields.Add(propertyInfo);
		}
	}
}

void UKlawrBlueprintGeneratedClass::GetScriptDefinedFunctions(TArray<FScriptFunction>& OutFunctions)
{
	OutFunctions.Empty();
	if (ScriptDefinedType.IsEmpty())
	{
		return;
	}

	std::vector<Klawr::tstring> functionNames;
	Klawr::IClrHost::Get()->GetScriptComponentFunctionNames(appDomainId, *ScriptDefinedType, functionNames);
	for (auto& functionName : functionNames)
	{
		const TCHAR* dummy = functionName.c_str();
		UE_LOG(LogKlawrRuntimePlugin, Warning, TEXT("Found Function %s"), dummy);
		std::vector<Klawr::tstring> functionParameterNames;
		Klawr::IClrHost::Get()->GetScriptComponentFunctionParameterNames(appDomainId, *ScriptDefinedType, dummy, functionParameterNames);
		FScriptFunction newFunction(dummy);
		// Get Result type first
		newFunction.ResultType = Klawr::IClrHost::Get()->GetScriptComponentFunctionParameterType(appDomainId, *ScriptDefinedType, dummy, -1);
		int paramCount = 0;
		for (auto& functionParameterName : functionParameterNames)
		{
			const TCHAR* dummy2 = functionParameterName.c_str();
			int parameterType = Klawr::IClrHost::Get()->GetScriptComponentFunctionParameterType(appDomainId, *ScriptDefinedType, dummy, paramCount);
			newFunction.Parameters.Add(dummy2, parameterType);
			if (parameterType == 4)
			{
				UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("UObjects as parameters are not supported yet. Please use local properties to pass UObjects into c# space. (Parameter '%s' of function '%s' in class '%s')"), 
					dummy2, 
					dummy, 
					*ScriptDefinedType);
				newFunction.parameterClasses.Add(FindObject<UClass>(ANY_PACKAGE, Klawr::IClrHost::Get()->GetScriptComponentFunctionParameterTypeObjectClass(appDomainId, *ScriptDefinedType, dummy, paramCount)));
			}
			else
			{
				newFunction.parameterClasses.Add(NULL);
			}
			paramCount++;
		}
		OutFunctions.Add(newFunction);
	}
}


