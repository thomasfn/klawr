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
#include "JsonObject.h"
#include "JsonObjectConverter.h"
#include "../../../ThirdParty/Klawr/ClrHostManaged/Wrappers/TypeTranslatorEnum.cs"


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

	const FCLRAssemblyInfo& assemblyInfo = IKlawrRuntimePlugin::Get().GetAssemblyInfo(appDomainId);
	// FJsonObjectConverter::JsonObjectStringToUStruct(FString(Klawr::IClrHost::Get()->GetAssemblyInfo(appDomainId)), &assemblyInfo, 0, 0);

	for (auto CLRClass : assemblyInfo.ClassInfos)
	{
		if (CLRClass.Name.Equals(ScriptDefinedType))
		{
			// Found the one we're looking for here
			for (auto CLRProperty : CLRClass.PropertyInfos)
			{
				FScriptField propertyInfo;
				switch (CLRProperty.TypeId)
				{
				case ParameterTypeTranslation::ParametertypeFloat:
					propertyInfo.Class = UFloatProperty::StaticClass();
					break;
				case ParameterTypeTranslation::ParametertypeInt:
					propertyInfo.Class = UIntProperty::StaticClass();
					break;
				case ParameterTypeTranslation::ParametertypeBool:
					propertyInfo.Class = UBoolProperty::StaticClass();
					break;
				case ParameterTypeTranslation::ParametertypeString:
					propertyInfo.Class = UStrProperty::StaticClass();
					break;
				case ParameterTypeTranslation::ParametertypeEnum:
				{
					propertyInfo.Class = UByteProperty::StaticClass();

					FString enumName = CLRProperty.ClassName;
					propertyInfo.innerEnum = FindObject<UEnum>(ANY_PACKAGE, *enumName);
					if (propertyInfo.innerEnum != nullptr) break;

					enumName.RemoveFromStart(L"E", ESearchCase::CaseSensitive);

					propertyInfo.innerEnum = FindObject<UEnum>(ANY_PACKAGE, *enumName);
					if (propertyInfo.innerClass != nullptr) break;

					UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("Could not locate UEnum for name '%s' in class '%s'"),
						*CLRProperty.ClassName, *CLRClass.Name);
					break;
				}
				case ParameterTypeTranslation::ParametertypeObject:
				{
					propertyInfo.Class = UObjectProperty::StaticClass();


					FString className = CLRProperty.ClassName;
					propertyInfo.innerClass = FindObject<UClass>(ANY_PACKAGE, *className);
					if (propertyInfo.innerClass != nullptr) break;

					className.RemoveFromStart(L"F", ESearchCase::CaseSensitive);

					propertyInfo.innerClass = FindObject<UClass>(ANY_PACKAGE, *className);
					if (propertyInfo.innerClass != nullptr) break;

					className.RemoveFromStart(L"U", ESearchCase::CaseSensitive);

					propertyInfo.innerClass = FindObject<UClass>(ANY_PACKAGE, *className);

					if (propertyInfo.innerClass != nullptr) break;

					UE_LOG(LogKlawrRuntimePlugin, Error, TEXT("Could not locate UClass for name '%s' in class '%s'"),
						*CLRProperty.ClassName, *CLRClass.Name);

					break;
				}
				}

				if (propertyInfo.Class)
				{
					propertyInfo.Name = FName(*CLRProperty.Name);
					for (auto meta : CLRProperty.MetaData)
					{
						propertyInfo.metas.Add(FString(meta.MetaKey), FString(meta.MetaValue));
					}
					OutFields.Add(propertyInfo);
				}
			}
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
	
	// FJsonObjectConverter::JsonObjectStringToUStruct(FString(Klawr::IClrHost::Get()->GetAssemblyInfo(appDomainId)), &assemblyInfo, 0, 0);
	const FCLRAssemblyInfo& assemblyInfo = IKlawrRuntimePlugin::Get().GetAssemblyInfo(appDomainId);

	for (auto CLRClass : assemblyInfo.ClassInfos)
	{
		if (CLRClass.Name.Equals(ScriptDefinedType))
		{
			// Found the one we're looking for here
			for (auto CLRMethod : CLRClass.MethodInfos)
			{
				FScriptFunction newFunction(*CLRMethod.Name);
				FScriptFunctionParam fresult(CLRMethod.ReturnType);
				switch (CLRMethod.ReturnType)
				{
					case ParameterTypeTranslation::ParametertypeObject:
						fresult.innerClass = FindObject<UClass>(ANY_PACKAGE, *CLRMethod.ClassName);
						break;
					case ParameterTypeTranslation::ParametertypeEnum:
						fresult.innerEnum = FindObject<UEnum>(ANY_PACKAGE, *CLRMethod.ClassName);
						break;
				}
				newFunction.Result = fresult;
				for (auto CLRParameter : CLRMethod.Parameters)
				{
					FScriptFunctionParam fparam(CLRParameter.TypeId);
					switch (CLRParameter.TypeId)
					{
						case ParameterTypeTranslation::ParametertypeObject:
							fparam.innerClass = FindObject<UClass>(ANY_PACKAGE, *CLRParameter.ClassName);
							break;
						case ParameterTypeTranslation::ParametertypeEnum:
							fparam.innerEnum = FindObject<UEnum>(ANY_PACKAGE, *CLRParameter.ClassName);
							break;
					}
					newFunction.Parameters.Add(CLRParameter.Name, fparam);
				}
				OutFunctions.Add(newFunction);
			}
			break;
		}
	}
}


