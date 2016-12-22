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
	auto bpClass = UKlawrBlueprintGeneratedClass::GetBlueprintGeneratedClass(GetClass());
	for (auto prop : bpClass->ScriptProperties)
	{
		Klawr::PropertyTracker tracker;
		tracker.Name = prop->GetFName().ToString();
		tracker.Property = prop;
		tracker.Type = Klawr::GetterSetter;
		tracker.ResetPrevious();
		propertyTrackers.Add(tracker);
	}

	if (!Proxy && !HasAnyFlags(RF_ClassDefaultObject))
	{
		CreateScriptComponentProxy();
	}

	if (Proxy)
	{
		// users don't have to implement InitializeComponent() in their scripts,
		// so here we figure out which of those have been implemented
		bWantsInitializeComponent = !!Proxy->InitializeComponent;

		// tick if the user implemented TickComponent OR we have properties to track
		PrimaryComponentTick.bCanEverTick = propertyTrackers.Num() > 0 || Proxy->TickComponent;
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
	propertyTrackers.Empty();

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

	if (Proxy)
	{
		for (int i = 0; i < propertyTrackers.Num(); i++)
		{
			UpdatePropertyTracker(propertyTrackers[i]);
		}
		if (Proxy->InitializeComponent) Proxy->InitializeComponent();
	}
}

void UKlawrScriptComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Proxy)
	{ 
		for (int i = 0; i < propertyTrackers.Num(); i++)
		{
			UpdatePropertyTracker(propertyTrackers[i]);
		}
		if (Proxy->TickComponent) Proxy->TickComponent(DeltaTime);
	}
}

bool streq(const TCHAR* a, const TCHAR* b)
{
	if (a == b) return true;
	if (a == NULL || b == NULL) return false;
	return wcscmp(a, b) == 0;
}

void UKlawrScriptComponent::UpdatePropertyTracker(Klawr::PropertyTracker& tracker)
{
	IKlawrRuntimePlugin& runtime = IKlawrRuntimePlugin::Get();
	const TCHAR* propertyName = *(tracker.Name);
	
	if (tracker.Property->GetClass()->IsChildOf(UBoolProperty::StaticClass()))
	{
		UBoolProperty* propBool = Cast<UBoolProperty>(tracker.Property);

		// Did managed value change?
		bool prevManagedValue = tracker.GetPreviousManaged<int>() == 1;
		bool managedValue = runtime.GetBool(appDomainId, Proxy->InstanceID, propertyName);
		if (prevManagedValue != managedValue)
		{
			// Managed value changed, commit it
			propBool->SetPropertyValue_InContainer(this, managedValue);
			tracker.SetPreviousManaged(managedValue ? 1 : 0);
			tracker.SetPreviousNative(managedValue ? 1 : 0);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (managed-side), was %s, now %s"), propertyName, prevManagedValue ? L"True" : L"False", managedValue ? L"True" : L"False");
			return;
		}

		// Did native value change?
		bool prevNativeValue = tracker.GetPreviousNative<int>() == 1;
		bool nativeValue = propBool->GetPropertyValue_InContainer(this);
		if (prevNativeValue != nativeValue)
		{
			// Native value changed, commit it
			runtime.SetBool(appDomainId, Proxy->InstanceID, propertyName, nativeValue);
			tracker.SetPreviousManaged(nativeValue ? 1 : 0);
			tracker.SetPreviousNative(nativeValue ? 1 : 0);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (native-side), was %s, now %s"), propertyName, prevNativeValue ? L"True" : L"False", nativeValue ? L"True" : L"False");
			return;
		}

				
	}
	else if (tracker.Property->GetClass()->IsChildOf(UIntProperty::StaticClass()))
	{
		UIntProperty* propInt = Cast<UIntProperty>(tracker.Property);
		
		// Did managed value change?
		int prevManagedValue = tracker.GetPreviousManaged<int>();
		int managedValue = runtime.GetInt(appDomainId, Proxy->InstanceID, propertyName);
		if (prevManagedValue != managedValue)
		{
			// Managed value changed, commit it
			propInt->SetPropertyValue_InContainer(this, managedValue);
			tracker.SetPreviousManaged(managedValue);
			tracker.SetPreviousNative(managedValue);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (managed-side), was %i, now %i"), propertyName, prevManagedValue, managedValue);
			return;
		}

		// Did native value change?
		int prevNativeValue = tracker.GetPreviousNative<int>();
		int nativeValue = propInt->GetPropertyValue_InContainer(this);
		if (prevNativeValue != nativeValue)
		{
			// Native value changed, commit it
			runtime.SetInt(appDomainId, Proxy->InstanceID, propertyName, nativeValue);
			tracker.SetPreviousManaged(nativeValue);
			tracker.SetPreviousNative(nativeValue);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (native-side), was %i, now %i"), propertyName, prevNativeValue, nativeValue);
			return;
		}
	}
	else if (tracker.Property->GetClass()->IsChildOf(UStrProperty::StaticClass()))
	{
		 UStrProperty* propStr = Cast<UStrProperty>(tracker.Property);

		 // Did managed value change?
		 const TCHAR* prevManagedValue = tracker.GetPreviousManaged<TCHAR*>();
		 const TCHAR* managedValue = runtime.GetStr(appDomainId, Proxy->InstanceID, propertyName);
		 if (!streq(prevManagedValue, managedValue))
		 {
			 // Managed value changed, commit it
			 propStr->SetPropertyValue_InContainer(this, managedValue);
			 tracker.SetPreviousManaged(managedValue);
			 tracker.SetPreviousNative(managedValue);
			 UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (managed-side), was %s, now %s"), propertyName, prevManagedValue, managedValue);
			 return;
		 }

		 // Did native value change?
		 const TCHAR* prevNativeValue = tracker.GetPreviousNative<TCHAR*>();
		 const TCHAR* nativeValue = *(propStr->GetPropertyValue_InContainer(this));
		 if (!streq(prevNativeValue, nativeValue))
		 {
			 // Native value changed, commit it
			 runtime.SetStr(appDomainId, Proxy->InstanceID, propertyName, nativeValue);
			 tracker.SetPreviousManaged(nativeValue);
			 tracker.SetPreviousNative(nativeValue);
			 UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (native-side), was %s, now %s"), propertyName, prevNativeValue, nativeValue);
			 return;
		 }
		// propStr->SetPropertyValue_InContainer(object, GetStr(appDomainID, instanceID, propertyName));

		// UStrProperty* propStr = Cast<UStrProperty>(prop);
		// SetStr(appDomainID, instanceID, propertyName, *(propStr->GetPropertyValue_InContainer(object)));
	}
	else if (tracker.Property->GetClass()->IsChildOf(UFloatProperty::StaticClass()))
	{
		UFloatProperty* propFloat = Cast<UFloatProperty>(tracker.Property);

		// Did managed value change?
		float prevManagedValue = tracker.GetPreviousManaged<float>();
		float managedValue = runtime.GetFloat(appDomainId, Proxy->InstanceID, propertyName);
		if (prevManagedValue != managedValue)
		{
			// Managed value changed, commit it
			propFloat->SetPropertyValue_InContainer(this, managedValue);
			tracker.SetPreviousManaged(managedValue);
			tracker.SetPreviousNative(managedValue);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (managed-side), was %f, now %f"), propertyName, prevManagedValue, managedValue);
			return;
		}

		// Did native value change?
		float prevNativeValue = tracker.GetPreviousNative<float>();
		float nativeValue = propFloat->GetPropertyValue_InContainer(this);
		if (prevNativeValue != nativeValue)
		{
			// Native value changed, commit it
			runtime.SetFloat(appDomainId, Proxy->InstanceID, propertyName, nativeValue);
			tracker.SetPreviousManaged(nativeValue);
			tracker.SetPreviousNative(nativeValue);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (native-side), was %f, now %f"), propertyName, prevNativeValue, nativeValue);
			return;
		}
	}
	else if (tracker.Property->GetClass()->IsChildOf(UObjectProperty::StaticClass()))
	{
		UObjectProperty* propObject = Cast<UObjectProperty>(tracker.Property);
		/*UObject* returnObject = GetObj(appDomainID, instanceID, propertyName);
		if ((returnObject != NULL) && (returnObject != nullptr))
		{
			propObject->SetObjectPropertyValue(prop->ContainerPtrToValuePtr<UObject*>(object), returnObject);
		}*/

		/*UObjectProperty* propObject = Cast<UObjectProperty>(prop);
		UObject* temp = propObject->GetObjectPropertyValue(prop->ContainerPtrToValuePtr<UObject*>(object));
		SetObj(appDomainID, instanceID, propertyName, propObject->GetObjectPropertyValue(prop->ContainerPtrToValuePtr<UObject*>(object)));*/

		// Did managed value change?
		UObject* prevManagedValue = tracker.GetPreviousManaged<UObject*>();
		UObject* managedValue = runtime.GetObj(appDomainId, Proxy->InstanceID, propertyName);
		if (prevManagedValue != managedValue)
		{
			// Managed value changed, commit it
			propObject->SetObjectPropertyValue(tracker.Property->ContainerPtrToValuePtr<UObject*>(this), managedValue);
			tracker.SetPreviousManaged(managedValue);
			tracker.SetPreviousNative(managedValue);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (managed-side), was %s, now %s"), propertyName, prevManagedValue != NULL ? *(prevManagedValue->GetFullName()) : L"null", managedValue != NULL ? *(managedValue->GetFullName()) : L"null");
			return;
		}

		// Did native value change?
		UObject* prevNativeValue = tracker.GetPreviousNative<UObject*>();
		UObject* nativeValue = propObject->GetObjectPropertyValue(tracker.Property->ContainerPtrToValuePtr<UObject*>(this));
		if (prevNativeValue != nativeValue)
		{
			// Native value changed, commit it
			runtime.SetObj(appDomainId, Proxy->InstanceID, propertyName, nativeValue);
			tracker.SetPreviousManaged(nativeValue);
			tracker.SetPreviousNative(nativeValue);
			UE_LOG(LogKlawrRuntimePlugin, Log, TEXT("Property %s changed (managed-side), was %s, now %s"), propertyName, prevNativeValue != NULL ? *(prevNativeValue->GetFullName()) : L"null", nativeValue != NULL ? *(nativeValue->GetFullName()) : L"null");
			return;
		}
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



