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

#include "KlawrCodeGeneratorPluginPrivatePCH.h"
#include "KlawrCSharpStructWrapperGenerator.h"
#include "KlawrCodeFormatter.h"
#include "KlawrCodeGenerator.h"

namespace Klawr {

	const FString FCSharpStructWrapperGenerator::UnmanagedFunctionPointerAttribute =
#ifdef UNICODE
		TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]");
#else
		TEXT("[UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]");
#endif // UNICODE
	const FString FCSharpStructWrapperGenerator::MarshalReturnedBoolAsUint8Attribute = TEXT("[return: MarshalAs(UnmanagedType.U1)]");
	const FString FCSharpStructWrapperGenerator::MarshalBoolParameterAsUint8Attribute = TEXT("[MarshalAs(UnmanagedType.U1)]");

	FCSharpStructWrapperGenerator::FCSharpStructWrapperGenerator(const UScriptStruct* Struct, FCodeFormatter& CodeFormatter)
		: Struct(Struct), GeneratedGlue(CodeFormatter)
	{
		Struct->GetName(FriendlyStructName);
		NativeStructName = FString::Printf(TEXT("%s%s"), Struct->GetPrefixCPP(), *FriendlyStructName);
		bShouldGenerateManagedWrapper = ShouldGenerateManagedWrapper(Struct);
	}

	void FCSharpStructWrapperGenerator::GenerateHeader()
	{
		GeneratedGlue
			// usings
			<< TEXT("using System;")
			<< TEXT("using System.Runtime.InteropServices;")
			<< TEXT("using System.Collections.Generic;")
			<< TEXT("using Klawr.ClrHost.Interfaces;")
			<< TEXT("using Klawr.ClrHost.Managed;")
			<< TEXT("using Klawr.ClrHost.Managed.SafeHandles;")
			<< TEXT("using Klawr.ClrHost.Managed.Collections;")
			<< TEXT("using Klawr.ClrHost.Managed.Attributes;")
			<< FCodeFormatter::LineTerminator()

			// declare namespace
			<< TEXT("namespace Klawr.UnrealEngine")
			<< FCodeFormatter::OpenBrace();

		if (bShouldGenerateManagedWrapper)
		{
			const FString packageName = Struct->GetOutermost()->GetName();
			GeneratedGlue
				<< FString::Printf(TEXT("// Package: %s"), *packageName)
				// declare struct
				<< TEXT("[StructLayout(LayoutKind.Sequential)]")
				<< FString::Printf(TEXT("public struct %s"), *NativeStructName)
				<< FCodeFormatter::OpenBrace();
		}
	}

	void FCSharpStructWrapperGenerator::GenerateFooter()
	{
		if (bShouldGenerateManagedWrapper)
		{
			// close the class
			GeneratedGlue
				<< FCodeFormatter::CloseBrace()
				<< FCodeFormatter::LineTerminator();
		}

		// close the namespace
		GeneratedGlue << FCodeFormatter::CloseBrace();
	}

	void FCSharpStructWrapperGenerator::GeneratePropertyWrapper(const UProperty* prop)
	{
		auto arrayProp = Cast<UArrayProperty>(prop);
		if (arrayProp)
		{
			GenerateArrayPropertyWrapper(arrayProp);
		}
		else
		{
			GenerateStandardPropertyWrapper(prop);
		}
	}

	void FCSharpStructWrapperGenerator::GenerateStandardPropertyWrapper(const UProperty* Property)
	{
		const bool bIsBoolProperty = Property->IsA<UBoolProperty>();
		const FString interopTypeName = GetPropertyInteropType(Property);
		FString managedTypeName = GetPropertyManagedType(Property);
		if (Property->IsA<UClassProperty>())
		{
			// TODO: Handle this properly!
			GeneratedGlue
				<< FString::Printf(TEXT("public IntPtr %s; // UClass*"), *Property->GetName());
		}
		else if (Property->IsA<UObjectProperty>())
		{
			GeneratedGlue
				<< FString::Printf(TEXT("private IntPtr _%s;"), *Property->GetName())
				<< FString::Printf(TEXT("public %s %s"), *managedTypeName, *Property->GetName())
				<< FCodeFormatter::OpenBrace()
				<< TEXT("get")
				<< FCodeFormatter::OpenBrace()
				<< FString::Printf(TEXT("if (_%s == IntPtr.Zero) return null;"), *Property->GetName())
				<< FString::Printf(
					TEXT("return new %s(new UObjectHandle(_%s, false));"),
					*managedTypeName, *Property->GetName())
				<< FCodeFormatter::CloseBrace()
				<< TEXT("set")
				<< FCodeFormatter::OpenBrace()
				<< FString::Printf(
					TEXT("_%s = value != null ? value.NativeObject.Handle : IntPtr.Zero;"),
					*Property->GetName())
				<< FCodeFormatter::CloseBrace()
				<< FCodeFormatter::CloseBrace();
		}
		else if (Property->IsA<UBoolProperty>())
		{
			GeneratedGlue
				<< MarshalBoolParameterAsUint8Attribute
				<< FString::Printf(TEXT("public %s %s;"), *interopTypeName, *Property->GetName());
		}
		else if (Property->IsA<UWeakObjectProperty>())
		{
			FString typeName = FCodeGenerator::GetPropertyCPPType(Property);
			typeName.RemoveFromStart(L"TWeakObjectPtr<");
			typeName.RemoveFromEnd(L">");
			GeneratedGlue
				<< FString::Printf(TEXT("private IntPtr _%s;"), *Property->GetName())
				<< FString::Printf(TEXT("public %s %s // UWeakObjectPtr"), *typeName, *Property->GetName())
				<< FCodeFormatter::OpenBrace()
				<< TEXT("get")
				<< FCodeFormatter::OpenBrace()
				<< FString::Printf(TEXT("if (_%s == IntPtr.Zero) return null;"), *Property->GetName())
				<< FString::Printf(
					TEXT("return new %s(new UObjectHandle(_%s, false));"),
					*typeName, *Property->GetName())
				<< FCodeFormatter::CloseBrace()
				<< TEXT("set")
				<< FCodeFormatter::OpenBrace()
				<< FString::Printf(
					TEXT("_%s = value != null ? value.NativeObject.Handle : IntPtr.Zero;"),
					*Property->GetName())
				<< FCodeFormatter::CloseBrace()
				<< FCodeFormatter::CloseBrace();
		}
		else
		{
			GeneratedGlue
				<< FString::Printf(TEXT("public %s %s;"), *interopTypeName, *Property->GetName());
		}
	}

	void FCSharpStructWrapperGenerator::GenerateArrayPropertyWrapper(const UArrayProperty* arrayProp)
	{
		// It's a TArray<T>, for now assume an int32 and a pointer (THIS IS PROBABLY WRONG)
		GeneratedGlue
			<< FString::Printf(TEXT("private int _%s_len;"), *arrayProp->GetName())
			<< FCodeFormatter::LineTerminator()
			<< FString::Printf(TEXT("private IntPtr _%s_data;"), *arrayProp->GetName())
			<< FCodeFormatter::LineTerminator();
		// TODO: Getter/setter for this
	}

	bool FCSharpStructWrapperGenerator::ShouldGenerateManagedWrapper(const UScriptStruct* Struct)
	{
		return true;
	}

	FString FCSharpStructWrapperGenerator::GetPropertyInteropType(const UProperty* Property)
	{
		if (Property->IsA<UObjectProperty>())
		{
			return TEXT("UObjectHandle");
		}
		else if (Property->IsA<UObjectPropertyBase>())
		{
			return TEXT("IntPtr");
		}
		else if (Property->IsA<UBoolProperty>())
		{
			return TEXT("bool");
		}
		else if (Property->IsA<UIntProperty>())
		{
			return TEXT("int");
		}
		else if (Property->IsA<UFloatProperty>())
		{
			return TEXT("float");
		}
		else if (Property->IsA<UDoubleProperty>())
		{
			return TEXT("double");
		}
		else if (Property->IsA<UStrProperty>())
		{
			return TEXT("string");
		}
		else if (Property->IsA<UNameProperty>())
		{
			return TEXT("FScriptName");
		}
		else
		{
			FString type = FCodeGenerator::GetPropertyCPPType(Property);
			while (type.Contains(L"::"))
			{
				// Assume it's an enum, pick the entry starting with E
				FString left, right;
				type.Split(L"::", &left, &right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				if (right[0] == L'E')
				{
					return right;
				}
				else
				{
					type = left;
				}
			}
			return type;
		}
	}

	FString FCSharpStructWrapperGenerator::GetPropertyManagedType(const UProperty* Property)
	{
		if (Property->IsA<UObjectProperty>())
		{
			static FString pointer(TEXT("*"));
			FString typeName = FCodeGenerator::GetPropertyCPPType(Property);
			typeName.RemoveFromEnd(pointer);
			return typeName;
		}
		return GetPropertyInteropType(Property);
	}

	FString FCSharpStructWrapperGenerator::GetPropertyInteropTypeAttributes(const UProperty* Property)
	{
		if (Property->IsA<UBoolProperty>())
		{
			// by default C# bool gets marshaled to unmanaged BOOL (4-bytes),
			// marshal it to uint8 instead (which is the size of an MSVC bool)
			return MarshalBoolParameterAsUint8Attribute;
		}

		// Not convinced the directional attributes are necessary, seems like the C# out and ref 
		// keywords should be sufficient.
		/*
		if (!Property->IsA<UObjectPropertyBase>())
		{
		if (!Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm))
		{
		if (Property->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
		Attributes += TEXT("[In][Out]");
		}
		else if (Property->HasAnyPropertyFlags(CPF_OutParm))
		{
		Attributes += TEXT("[Out]");
		}
		}
		}
		*/
		return FString();
	}

	FString FCSharpStructWrapperGenerator::GetPropertyInteropTypeModifiers(const UProperty* Property)
	{
		if (!Property->IsA<UObjectPropertyBase>())
		{
			if (!Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm))
			{
				if (Property->HasAnyPropertyFlags(CPF_ReferenceParm))
				{
					return TEXT("ref");
				}
				else if (Property->HasAnyPropertyFlags(CPF_OutParm))
				{
					return TEXT("out");
				}
			}
		}
		return FString();
	}

	FString FCSharpStructWrapperGenerator::GetDelegateTypeName(
		const FString& FunctionName, bool bHasReturnValue
	)
	{
		if (bHasReturnValue)
		{
			return FunctionName + TEXT("Func");
		}
		else
		{
			return FunctionName + TEXT("Action");
		}
	}

	FString FCSharpStructWrapperGenerator::GetDelegateName(const FString& FunctionName)
	{
		return FString(TEXT("_")) + FunctionName;
	}

	FString FCSharpStructWrapperGenerator::GetArrayPropertyWrapperType(const UArrayProperty* arrayProperty)
	{
		const UProperty* elementProperty = arrayProperty->Inner;
		if (elementProperty->IsA<UStrProperty>())
		{
			return TEXT("StringArrayProperty");
		}
		else if (elementProperty->IsA<UNameProperty>())
		{
			return TEXT("NameArrayProperty");
		}
		else if (elementProperty->IsA<UObjectProperty>())
		{
			return FString::Printf(
				TEXT("ObjectArrayProperty<%s>"), *GetPropertyManagedType(elementProperty)
			);
		}
		else if (elementProperty->IsA<UBoolProperty>())
		{
			return TEXT("BoolArrayProperty");
		}
		else
		{
			// Convert "int32" to "Int32"
			FString rawTypeName = elementProperty->GetCPPType();
			rawTypeName.ToLowerInline();
			rawTypeName[0] = towupper(rawTypeName[0]);
			return FString::Printf(TEXT("%sArrayProperty"), *rawTypeName);
		}
	}

} // namespace Klawr
