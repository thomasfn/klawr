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

#include "KlawrScriptComponent.generated.h"

namespace Klawr
{
	struct ScriptComponentProxy;

	enum PropertyTrackerType { GetterSetter };

	struct PropertyTracker
	{
		FString Name;
		UProperty* Property;
		PropertyTrackerType Type;

		uint8 PreviousNative[8];
		uint8 PreviousManaged[8];

		template <typename T> T GetPreviousNative() const { return *(reinterpret_cast<const T*>(PreviousNative)); }
		template <typename T> T GetPreviousManaged() const { return *(reinterpret_cast<const T*>(PreviousManaged)); }
		template <typename T> void SetPreviousNative(T value) { *(reinterpret_cast<T*>(PreviousNative)) = value; }
		template <typename T> void SetPreviousManaged(T value) { *(reinterpret_cast<T*>(PreviousManaged)) = value; }

		void ResetPrevious()
		{
			ZeroMemory(PreviousNative, 8);
			ZeroMemory(PreviousManaged, 8);
		}
	};
} // namespace Klawr

/**
 * A component whose functionality is implemented in C# or any other CLI language.
 */
UCLASS(Blueprintable, hidecategories = (Object, ActorComponent), editinlinenew, meta = (BlueprintSpawnableComponent), ClassGroup = Script, Abstract)
class KLAWRRUNTIMEPLUGIN_API UKlawrScriptComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKlawrScriptComponent(const FObjectInitializer& objectInitializer);

public: // UActorComponent interface
	
	/** 
	 * Called after OnComponentCreated() has been called for all default (native) components 
	 * attached to the actor.
	 */
	virtual void OnRegister() override;
	
	/** Called before OnComponentDestroyed() if the component is registered. */
	virtual void OnUnregister() override;
	
	/**
	 * Begin gameplay.
	 * At this point OnRegister() has been called on all components attached to the actor.
	 */
	virtual void InitializeComponent() override;
	
	/** Update the state of the component. */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "Klawr")
	virtual float CallCSFunctionFloat(FString functionName, UKlawrArgArray* args);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "Klawr")
	virtual int32 CallCSFunctionInt(FString functionName, UKlawrArgArray* args);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "Klawr")
	virtual bool CallCSFunctionBool(FString functionName, UKlawrArgArray* args);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "Klawr")
	virtual FString CallCSFunctionString(FString functionName, UKlawrArgArray* args);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "Klawr")
	virtual UObject* CallCSFunctionObject(FString functionName, UKlawrArgArray* args);
	UFUNCTION(meta = (BlueprintInternalUseOnly = "true"), BlueprintCallable, Category = "Klawr")
	virtual void CallCSFunctionVoid(FString functionName, UKlawrArgArray* args);


private:
	void CreateScriptComponentProxy();
	void DestroyScriptComponentProxy();

	void UpdatePropertyTracker(Klawr::PropertyTracker& tracker);

	int appDomainId = 0;
private:
	// a proxy that represents the managed counterpart of this script component
	Klawr::ScriptComponentProxy* Proxy;

	// all property trackers for this component
	TArray<Klawr::PropertyTracker> propertyTrackers;
};
