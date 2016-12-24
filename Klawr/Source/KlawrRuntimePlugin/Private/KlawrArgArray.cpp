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
}

void UKlawrArgArray::AddInt(int value)
{
	FKlawrVariantArg arg;
	arg.Type = KlawrVariantArgType::Int;
	arg.Data[0] = value;
	arg.Data[1] = 0;
	args.Add(arg);
}

void UKlawrArgArray::AddFloat(float value)
{
	FKlawrVariantArg arg;
	arg.Type = KlawrVariantArgType::Float;
	*(reinterpret_cast<float*>(arg.Data)) = value;
	arg.Data[1] = 0;
	args.Add(arg);
}

void UKlawrArgArray::AddBool(bool value)
{
	FKlawrVariantArg arg;
	arg.Type = KlawrVariantArgType::Bool;
	arg.Data[0] = value ? 1 : 0;
	arg.Data[1] = 0;
	args.Add(arg);
}

void UKlawrArgArray::AddString(FString value)
{
	FKlawrVariantArg arg;
	arg.Type = KlawrVariantArgType::String;
	*(reinterpret_cast<const TCHAR**>(arg.Data)) = *value;
	args.Add(arg);
}

void UKlawrArgArray::AddObject(UObject* value)
{
	FKlawrVariantArg arg;
	arg.Type = KlawrVariantArgType::Object;
	*(reinterpret_cast<UObject**>(arg.Data)) = value;
	args.Add(arg);
}