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
#include "KlawrCSharpEnumWrapperGenerator.h"
#include "KlawrCodeFormatter.h"
#include "KlawrCodeGenerator.h"

namespace Klawr {

	FCSharpEnumWrapperGenerator::FCSharpEnumWrapperGenerator(const UEnum* Enum, FCodeFormatter& CodeFormatter)
		: Enum(Enum), GeneratedGlue(CodeFormatter)
	{
		Enum->GetName(NativeEnumName);
	}

	void FCSharpEnumWrapperGenerator::GenerateHeader()
	{
		const FString packageName = Enum->GetOutermost()->GetName();
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
			<< FCodeFormatter::OpenBrace()

			<< FString::Printf(TEXT("// Package: %s"), *packageName)

			// declare enum
			<< FString::Printf(TEXT("public enum %s : byte"), *NativeEnumName) // TODO: Handle enums bigger than 8 bits - do they even exist?
			<< FCodeFormatter::OpenBrace();
	}

	void FCSharpEnumWrapperGenerator::GenerateFooter()
	{
		// close the class
		GeneratedGlue
			<< FCodeFormatter::CloseBrace()
			<< FCodeFormatter::LineTerminator()

			// close the namespace
			<< FCodeFormatter::CloseBrace();
	}

	void FCSharpEnumWrapperGenerator::GenerateName(const FString name, int value)
	{
		// If the name is namespaced, strip it
		if (name.Contains(L"::"))
		{
			FString left, right;
			name.Split(L"::", &left, &right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			GeneratedGlue
				<< FString::Printf(TEXT("%s = %i,"), *right, value);
		}
		else
		{
			GeneratedGlue
				<< FString::Printf(TEXT("%s = %i,"), *name, value);
		}
		
		NumNames++;
	}

} // namespace Klawr
