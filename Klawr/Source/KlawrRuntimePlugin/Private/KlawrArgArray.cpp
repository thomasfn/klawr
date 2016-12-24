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
#include "KlawrArgArray.h"

UKlawrArgArray* UKlawrArgArray::Create()
{
	return NewObject<UKlawrArgArray>();
}

void UKlawrArgArray::Clear()
{
	args.Empty();
	refStrings.Empty();
}

void UKlawrArgArray::AddInt(int value)
{
	Klawr::VariantArg arg;
	arg.Type = Klawr::VariantArgType::Int;
	arg.Data[0] = value;
	arg.Data[1] = 0;
	args.Add(arg);
}

void UKlawrArgArray::AddFloat(float value)
{
	Klawr::VariantArg arg;
	arg.Type = Klawr::VariantArgType::Float;
	*(reinterpret_cast<float*>(arg.Data)) = value;
	arg.Data[1] = 0;
	args.Add(arg);
}

void UKlawrArgArray::AddBool(bool value)
{
	Klawr::VariantArg arg;
	arg.Type = Klawr::VariantArgType::Bool;
	arg.Data[0] = value ? 1 : 0;
	arg.Data[1] = 0;
	args.Add(arg);
}

void UKlawrArgArray::AddString(FString value)
{
	// Store the string in refStrings, so that the buffer doesn't get deallocated while we're using it
	int idx = refStrings.Add(value);
	Klawr::VariantArg arg;
	arg.Type = Klawr::VariantArgType::String;
	*(reinterpret_cast<const TCHAR**>(arg.Data)) = *refStrings[idx];
	args.Add(arg);
}

void UKlawrArgArray::AddObject(UObject* value)
{
	Klawr::VariantArg arg;
	arg.Type = Klawr::VariantArgType::Object;
	*(reinterpret_cast<UObject**>(arg.Data)) = value;
	args.Add(arg);
}