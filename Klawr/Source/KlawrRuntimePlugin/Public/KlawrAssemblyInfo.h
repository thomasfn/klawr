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

#include "KlawrAssemblyInfo.generated.h"

USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLRMeta
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString MetaKey;

	UPROPERTY()
		FString MetaValue;
};

USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLRTypeInfo
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString Name;

	UPROPERTY()
		int32 TypeId;

	UPROPERTY()
		FString ClassName;

	UPROPERTY()
		TArray<FCLRMeta> MetaData;
};

USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLRMethodInfo
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString Name;

	UPROPERTY()
		int32 ReturnType;

	UPROPERTY()
		FString ClassName;

	UPROPERTY()
		TArray<FCLRTypeInfo> Parameters;

	UPROPERTY()
		TArray<FCLRMeta> MetaData;
};

USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLRClassInfo
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString Name;

	UPROPERTY()
		TArray<FCLRMethodInfo> MethodInfos;

	UPROPERTY()
		TArray<FCLRTypeInfo> PropertyInfos;
};

USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLREnumKeyValue
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString Key;

	UPROPERTY()
		int Value;
};

USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLREnumInfo
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FString Name;

	UPROPERTY()
		TArray<FCLREnumKeyValue> Values;
};


USTRUCT()
struct KLAWRRUNTIMEPLUGIN_API FCLRAssemblyInfo
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TArray<FCLRClassInfo> ClassInfos;

	UPROPERTY()
		TArray<FCLREnumInfo> EnumInfos;
};