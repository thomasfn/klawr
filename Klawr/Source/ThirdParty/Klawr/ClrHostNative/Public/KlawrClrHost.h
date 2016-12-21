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

// ugh, really don't like forcing this STL dependency on client code, but being constrained to 
// fixed size arrays of c-strings is extremely annoying
#include <vector>
#include <string>

namespace Klawr {

#ifdef _UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif // _UNICODE

} // namespace Klawr

#include "KlawrNativeUtils.h"

namespace Klawr {

/**
 * @brief Makes a copy of the given string, the resulting copy can be safely released by the CLR.
 *
 * The CLR will attempt to release any c-string that is returned to it from a native function after
 * it creates a corresponding managed string. However, it can only release the c-string if it was 
 * allocated on the correct heap. This function will allocate a copy of the passed in c-string on 
 * the same heap the CLR will attempt to release it from, this copy is what should be returned to 
 * the CLR instead of the original c-string.
 *
 * @return A copy of the c-string passed in.
 */
TCHAR* MakeStringCopyForCLR(const TCHAR* stringToCopy);

/**
 * @brief Contains native/managed interop information for a ScriptObject instance.
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Managed,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr::Managed namespace (but it's hidden from clients of this library).
 */
struct ScriptObjectInstanceInfo
{
	typedef void (*BeginPlayAction)();
	typedef void (*TickAction)(float);
	typedef void (*DestroyAction)();

	/** Unique ID of a managed ScriptObject instance. */
	__int64 InstanceID;
	/** Pointer to the managed BeginPlay() method of a ScriptObject instance. */
	BeginPlayAction BeginPlay;
	/** Pointer to the managed Tick() method of a ScriptObject instance. */
	TickAction Tick;
	/** Pointer to the managed Destroy() method of a ScriptObject instance. */
	DestroyAction Destroy;
};

/**
 * @brief A native proxy for a managed UKlawrScriptComponent instance.
 * 
 * The function pointers in the proxy are bound to the methods of a managed UKlawrScriptComponent 
 * instance. Managed UKlawrScriptComponent subclasses may not implement all methods, so some
 * of the function pointers may be null.
 *
 * @note This struct has a managed counterpart by the same name defined in Klawr.ClrHost.Managed,
 *       the managed counterpart is also exposed to native code via COM under the 
 *       Klawr::Managed namespace (but it's hidden from clients of this library). 
 *       The size and layout of the two structures must remain identical.
 */
struct ScriptComponentProxy
{
	typedef void (*OnComponentCreatedAction)();
	typedef void (*OnComponentDestroyedAction)();
	typedef void (*OnRegisterAction)();
	typedef void (*OnUnregisterAction)();
	typedef void (*InitializeComponentAction)();
	typedef void (*TickComponentAction)(float);

	/** Unique ID of the managed UKlawrScriptComponent instance this proxy represents. */
	__int64 InstanceID;
	/** Bound to UKlawrScriptComponent.OnComponentCreated() (may be null). */
	OnComponentCreatedAction OnComponentCreated;
	/** Bound to UKlawrScriptComponent.OnComponentDestroyed() (may be null). */
	OnComponentDestroyedAction OnComponentDestroyed;
	/** Bound to UKlawrScriptComponent.OnRegister() (may be null). */
	OnRegisterAction OnRegister;
	/** Bound to UKlawrScriptComponent.OnUnregister() (may be null). */
	OnUnregisterAction OnUnregister;
	/** Bound to UKlawrScriptComponent.InitializeComponent() (may be null). */
	InitializeComponentAction InitializeComponent;
	/** Bound to UKlawrScriptComponent.TickComponent() (may be null). */
	TickComponentAction TickComponent;
};

/** This public interface can be used to pass native wrapper functions to the CLR host. */
class IClrHost
{
public:
	/** 
	 * @brief Startup the CLR.
	 * @param engineAppDomainAppBase Base path for engine app domains, any assemblies to be loaded 
	 *        by the engine app domains must reside in this directory. If an empty/null string is 
	 *        passed in then the default application base is used (which will be the directory where
	 *        the application executable is located).
	 * @param gameScriptsAssemblyName The name of the assembly containing game scripts, it will be
	 *        automatically loaded into each engine app domain.
	 * @return true if the CLR was started up successfully, false otherwise
	 */
	virtual bool Startup(
		const TCHAR* engineAppDomainAppBase, const TCHAR* gameScriptsAssemblyName
	) = 0;

	/**
	 * @brief Create a new engine app domain.
	 * @param outAppDomainID Will be set to the ID of the newly created engine app domain.
	 * @return true if the app domain was created successfully, false otherwise
	 */
	virtual bool CreateEngineAppDomain(int& outAppDomainID) = 0;
	
	/**
	 * @brief Initialize a previously created engine app domain.
	 * @param appDomainID ID of the app domain to be initialized.
	 * @return true if the app domain was initialized successfully, false otherwise
	 */
	virtual bool InitEngineAppDomain(int appDomainID, const NativeUtils& nativeUtils) = 0;

	/**
	 * @brief Destroy an engine app domain.
	 * @param appDomainID The ID of the engine app domain to destroy.
	 * @return true if the app domain was destroyed successfully, false otherwise
	 */
	virtual bool DestroyEngineAppDomain(int appDomainID) = 0;
	
	/** Shutdown the CLR. */
	virtual void Shutdown() = 0;

	/** 
	 * @brief Store native wrapper functions for the given class.
	 * @param className The name of a scriptable C++ class (including prefix, e.g. AActor).
	 * @param wrapperFunctions Array of pointers to native wrapper functions for the given class.
	 * @param numFunctions Number of elements in the wrapperFunctions array.
	 */
	virtual void AddClass(const TCHAR* className, void** wrapperFunctions, int numFunctions) = 0;

	virtual bool CreateScriptObject(
		int appDomainID, const TCHAR* className, class UObject* owner, ScriptObjectInstanceInfo& info
	) = 0;

	virtual void DestroyScriptObject(int appDomainID, __int64 instanceID) = 0;

	/**
	 * @brief Create an instance of a managed UKlawrScriptComponent subclass.
	 * @param className The name of a managed UKlawrScriptComponent subclass.
	 * @param nativeComponent The native UKlawrScriptComponent instance to associate with the 
	 *                        managed instance.
	 * @param proxy If the managed instance is created successfully the function pointers in this
	 *              structure will be bound to the corresponding methods of the managed instance.
	 * @return true if the managed script component instance was created successfully, false otherwise
	 */
	virtual bool CreateScriptComponent(
		int appDomainID, const TCHAR* className, class UObject* nativeComponent, 
		ScriptComponentProxy& proxy
	) = 0;

	virtual void DestroyScriptComponent(int appDomainID, __int64 instanceID) = 0;

	/**
	 * @brief Get the fully qualified names (including namespace) of all currently loaded managed 
	 *        types derived from UKlawrScriptComponent.
	 * @param types Vector to be filled in with type names.
	 */
	virtual void GetScriptComponentTypes(int appDomainID, std::vector<tstring>& types) const = 0;

	virtual bool GetScriptComponentPropertyIsAdvancedDisplay(int appDomainID, const TCHAR* typeName, const TCHAR* propertyName) const = 0;
	virtual bool GetScriptComponentPropertyIsSaveGame(int appDomainID, const TCHAR* typeName, const TCHAR* propertyName) const = 0;

	virtual void SetFloat(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, float value) const = 0;
	virtual void SetInt(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, int value) const = 0;
	virtual void SetBool(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, bool value) const = 0;
	virtual void SetStr(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, const TCHAR* value) const = 0;
	virtual void SetObj(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, UObject* value) const = 0;

	virtual float GetFloat(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const = 0;
	virtual int GetInt(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const = 0;
	virtual bool GetBool(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const = 0;
	virtual const TCHAR* GetStr(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const = 0;
	virtual UObject* GetObj(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const = 0;
	virtual float CallCSFunctionFloat(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const = 0;
	virtual int CallCSFunctionInt(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const = 0;
	virtual bool CallCSFunctionBool(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const = 0;
	virtual const TCHAR* CallCSFunctionString(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const = 0;
	virtual UObject* CallCSFunctionObject(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const = 0;
	virtual void CallCSFunctionVoid(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<LONGLONG> objects) const = 0;

	virtual const TCHAR* GetAssemblyInfo(int appDomainID) const = 0;
public:
	/** Get the singleton instance. */
	static IClrHost* Get();

};

} // namespace Klawr