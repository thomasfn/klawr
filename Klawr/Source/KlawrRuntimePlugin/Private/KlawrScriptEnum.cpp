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
#include "KlawrScriptEnum.h"
#include "KlawrClrHost.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Kismet2/EnumEditorUtils.h"
#endif	// WITH_EDITOR

UKlawrScriptEnum::UKlawrScriptEnum(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	
}

void UKlawrScriptEnum::RebuildFromType()
{
	const FCLRAssemblyInfo& assemblyInfo = IKlawrRuntimePlugin::Get().GetPrimaryAssemblyInfo();

	TArray < TPair < FName, uint8 >> enumerators;
	for (const FCLREnumInfo& enumInfo : assemblyInfo.EnumInfos)
	{
		if (enumInfo.Name == ScriptDefinedType)
		{

			for (const FCLREnumKeyValue& keyValue : enumInfo.Values)
			{
				TPair < FName, uint8 > pair;
				const FString fullName = FString::Printf(TEXT("%s::%s"), *ScriptDefinedType, *keyValue.Key);
				pair.Key = FName(*fullName);
				pair.Value = keyValue.Value;
				enumerators.Add(pair);
			}

			break;
		}
	}

	SetEnums(enumerators, UEnum::ECppForm::Namespaced);
#if WITH_EDITOR
	FEnumEditorUtils::UpdateAfterPathChanged(this);
#endif	// WITH_EDITOR
}