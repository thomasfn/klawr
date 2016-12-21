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
#include "KlawrScriptComponent.h"
#include "KlawrClrHost.h"
#include "KlawrBlueprintGeneratedClass.h"

UKlawrScriptComponent::UKlawrScriptComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
	, Proxy(nullptr)
{
	// by default disable everything, re-enable only the relevant bits in OnRegister()
	PrimaryComponentTick.bCanEverTick = false;
	bTickInEditor = false;
	bAutoActivate = false;
	bWantsInitializeComponent = false;
	this->GetClass()->GetDefaultObject();
}

void UKlawrScriptComponent::CreateScriptComponentProxy()
{
	check(!Proxy);

	auto GeneratedClass = UKlawrBlueprintGeneratedClass::GetBlueprintGeneratedClass(GetClass());
	if (GeneratedClass)
	{
		Proxy = new Klawr::ScriptComponentProxy();
		appDomainId = IKlawrRuntimePlugin::Get().GetObjectAppDomainID(this);
		bool bCreated = Klawr::IClrHost::Get()->CreateScriptComponent(
			appDomainId,
			*GeneratedClass->ScriptDefinedType, this, *Proxy
		);

		if (!bCreated)
		{
			delete Proxy;
			Proxy = nullptr;
		}
		else
		{
			check(Proxy->InstanceID != 0);
		}
	}
}

void UKlawrScriptComponent::DestroyScriptComponentProxy()
{
	check(Proxy);

	if (Proxy->InstanceID != 0)
	{
		Klawr::IClrHost::Get()->DestroyScriptComponent(
			appDomainId, Proxy->InstanceID
		);
	}

	delete Proxy;
	Proxy = nullptr;
}

void UKlawrScriptComponent::OnRegister()
{
	if (!Proxy && !HasAnyFlags(RF_ClassDefaultObject))
	{
		CreateScriptComponentProxy();
	}

	if (Proxy)
	{
		// users don't have to implement InitializeComponent() and TickComponent() in their scripts,
		// so here we figure out which of those have been implemented
		bWantsInitializeComponent = !!Proxy->InitializeComponent;
		PrimaryComponentTick.bCanEverTick = !!Proxy->TickComponent;
		bAutoActivate = PrimaryComponentTick.bCanEverTick;

		if (Proxy->OnRegister)
		{
			Proxy->OnRegister();
		}
	}

	// this needs to be called after the code above because it's going to look at bAutoActivate
	Super::OnRegister();
}

void UKlawrScriptComponent::OnUnregister()
{
	if (Proxy)
	{
		if (Proxy->OnUnregister)
		{
			Proxy->OnUnregister();
		}
	
		DestroyScriptComponentProxy();
	}

	Super::OnUnregister();
}

void UKlawrScriptComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (Proxy && Proxy->InitializeComponent)
	{
		auto bpClass = UKlawrBlueprintGeneratedClass::GetBlueprintGeneratedClass(GetClass());
		IKlawrRuntimePlugin::Get().PushAllProperties(appDomainId, Proxy->InstanceID, this);
		Proxy->InitializeComponent();
		IKlawrRuntimePlugin::Get().PopAllProperties(appDomainId, Proxy->InstanceID, this);
	}
}

void UKlawrScriptComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Proxy && Proxy->TickComponent)
	{ 
		auto bpClass = UKlawrBlueprintGeneratedClass::GetBlueprintGeneratedClass(GetClass());
		IKlawrRuntimePlugin::Get().PushAllProperties(appDomainId, Proxy->InstanceID, this);
		Proxy->TickComponent(DeltaTime);
		IKlawrRuntimePlugin::Get().PopAllProperties(appDomainId, Proxy->InstanceID, this);
	}

}

float UKlawrScriptComponent::CallCSFunctionFloat(FString functionName, TArray<float> floats, TArray<int32> ints, TArray<bool> bools, TArray<FString> strings, TArray<UObject*> objects) 
{
	return IKlawrRuntimePlugin::Get().CallCSFunctionFloat(appDomainId, Proxy->InstanceID, *functionName, floats, ints, bools, strings, objects);
}

int32 UKlawrScriptComponent::CallCSFunctionInt(FString functionName, TArray<float> floats, TArray<int32> ints, TArray<bool> bools, TArray<FString> strings, TArray<UObject*> objects) 
{
	return IKlawrRuntimePlugin::Get().CallCSFunctionInt(appDomainId, Proxy->InstanceID, *functionName, floats, ints, bools, strings, objects);
}

bool UKlawrScriptComponent::CallCSFunctionBool(FString functionName, TArray<float> floats, TArray<int32> ints, TArray<bool> bools, TArray<FString> strings, TArray<UObject*> objects) 
{
	return IKlawrRuntimePlugin::Get().CallCSFunctionBool(appDomainId, Proxy->InstanceID, *functionName, floats, ints, bools, strings, objects);
}

FString UKlawrScriptComponent::CallCSFunctionString(FString functionName, TArray<float> floats, TArray<int32> ints, TArray<bool> bools, TArray<FString> strings, TArray<UObject*> objects) 
{
	return FString(IKlawrRuntimePlugin::Get().CallCSFunctionString(appDomainId, Proxy->InstanceID, *functionName, floats, ints, bools, strings, objects));
}

UObject* UKlawrScriptComponent::CallCSFunctionObject(FString functionName, TArray<float> floats, TArray<int32> ints, TArray<bool> bools, TArray<FString> strings, TArray<UObject*> objects)
{
	return IKlawrRuntimePlugin::Get().CallCSFunctionObject(appDomainId, Proxy->InstanceID, *functionName, floats, ints, bools, strings, objects);
}

void UKlawrScriptComponent::CallCSFunctionVoid(FString functionName, TArray<float> floats, TArray<int32> ints, TArray<bool> bools, TArray<FString> strings, TArray<UObject*> objects)
{
	IKlawrRuntimePlugin::Get().CallCSFunctionVoid(appDomainId, Proxy->InstanceID, *functionName, floats, ints, bools, strings, objects);
}



