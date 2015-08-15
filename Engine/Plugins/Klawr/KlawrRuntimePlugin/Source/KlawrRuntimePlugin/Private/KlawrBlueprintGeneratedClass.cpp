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


void UKlawrBlueprintGeneratedClass::GetScriptDefinedFields(TArray<FScriptField>& OutFields)
{
	if (!ScriptDefinedType.IsEmpty())
	{
		return;
	}
	// Read properties from generated class
	std::vector<Klawr::tstring> propertyNames;
	// We are most certainly in the editor atm
	Klawr::IClrHost::Get()->GetScriptComponentProperties(appDomainId, *ScriptDefinedType, propertyNames);
	OutFields.Empty();
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
		}

		// TODO: Add Objects

		if (propertyInfo.Class)
		{
			propertyInfo.Name = FName(propertyName.c_str());
			OutFields.Add(propertyInfo);
		}
	}
}