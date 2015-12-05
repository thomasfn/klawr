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

#include "KlawrClrHostPCH.h"
#include "ClrHost.h"
#include "ClrHostControl.h"
#include <metahost.h>
#include "KlawrClrHostInterfaces.h"

#pragma comment(lib, "mscoree.lib")

namespace {
	 
SAFEARRAY* CreateSafeArrayOfWrapperFunctions(void** wrapperFunctions, int numFunctions)
{
	SAFEARRAY* safeArray = SafeArrayCreateVector(VT_I8, 0, numFunctions);
	if (safeArray)
	{
		LONGLONG* safeArrayData = nullptr;
		HRESULT hr = SafeArrayAccessData(safeArray, (void**)&safeArrayData);
		if (SUCCEEDED(hr))
		{
			for (auto i = 0; i < numFunctions; ++i)
			{
				safeArrayData[i] = reinterpret_cast<LONGLONG>(wrapperFunctions[i]);
			}
			hr = SafeArrayUnaccessData(safeArray);
			assert(SUCCEEDED(hr));
		}
	}
	return safeArray;
}

/** 
 * @brief Convert a one-dimensional COM SAFEARRAY to a std::vector.
 * This only works if TSafeArrayElement can be implicitly converted to TVectorElement.
 */
template<typename TSafeArrayElement, typename TVectorElement>
void __cdecl SafeArrayToVector(SAFEARRAY* input, std::vector<TVectorElement>& output)
{
	LONG lowerBound, upperBound;
	HRESULT hr = SafeArrayGetLBound(input, 1, &lowerBound);
	if (FAILED(hr))
	{
		return;
	}
	hr = SafeArrayGetUBound(input, 1, &upperBound);
	if (FAILED(hr))
	{
		return;
	}
	LONG numElements = upperBound - lowerBound + 1;

	output.reserve(numElements);
	TSafeArrayElement* safeArrayData = nullptr;
	hr = SafeArrayAccessData(input, (void**)&safeArrayData);
	if (SUCCEEDED(hr))
	{
		for (auto i = 0; i < numElements; ++i)
		{
			output.push_back(safeArrayData[i]);
		}
		hr = SafeArrayUnaccessData(input);
		assert(SUCCEEDED(hr));
	}
}


template <class vectorType, VARTYPE safeArrayType>
void CreateSafeArray
(
	vectorType* lpT,
	ULONG ulSize,
	SAFEARRAY** ppSafeArrayReceiver
	)
{
	if ((lpT == NULL) || (ppSafeArrayReceiver == NULL))
	{
		// lpT and ppSafeArrayReceiver each cannot be NULL.
		return;
	}

	HRESULT hrRetTemp = S_OK;
	SAFEARRAYBOUND rgsabound[1];
	ULONG	ulIndex = 0;
	long lRet = 0;

	// Initialise receiver.
	*ppSafeArrayReceiver = NULL;

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = ulSize;

	*ppSafeArrayReceiver = (SAFEARRAY*)SafeArrayCreate
		(
			(VARTYPE)safeArrayType,
			(unsigned int)1,
			(SAFEARRAYBOUND*)rgsabound
			);

	for (ulIndex = 0; ulIndex < ulSize; ulIndex++)
	{
		long lIndexVector[1];

		lIndexVector[0] = ulIndex;

		SafeArrayPutElement
			(
				(SAFEARRAY*)(*ppSafeArrayReceiver),
				(long*)lIndexVector,
				(void*)(&(lpT[ulIndex]))
				);
	}

	return;
}
} // unnamed namespace

namespace Klawr {

struct ProxySizeChecks
{
	static_assert(
		sizeof(Klawr::Managed::ObjectUtilsProxy) == sizeof(ObjectUtilsProxy),
		"ObjectUtilsProxy doesn't have the same size in native and managed code!"
	);

	static_assert(
		sizeof(Klawr::Managed::LogUtilsProxy) == sizeof(LogUtilsProxy),
		"LogUtilsProxy doesn't have the same size in native and managed code!"
	);

	static_assert(
		sizeof(Klawr::Managed::ArrayUtilsProxy) == sizeof(ArrayUtilsProxy),
		"ArrayUtilsProxy doesn't have the same size in native and managed code!"
	);

	static_assert(
		sizeof(Klawr::Managed::ScriptComponentProxy) == sizeof(ScriptComponentProxy),
		"ScriptComponentProxy doesn't have the same size in native and managed code!"
	);

};

bool __cdecl ClrHost::Startup(const TCHAR* engineAppDomainAppBase, const TCHAR* gameScriptsAssemblyName)
{
	_COM_SMARTPTR_TYPEDEF(ICLRMetaHost, IID_ICLRMetaHost);
	_COM_SMARTPTR_TYPEDEF(ICLRRuntimeInfo, IID_ICLRRuntimeInfo);
	_COM_SMARTPTR_TYPEDEF(ICLRControl, IID_ICLRControl);

	_engineAppDomainAppBase = engineAppDomainAppBase;
	_gameScriptsAssemblyName = gameScriptsAssemblyName;

	// bootstrap the CLR
	
	ICLRMetaHostPtr metaHost;
	HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&metaHost));
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// specify which version of the CLR should be used
	ICLRRuntimeInfoPtr runtimeInfo;
	hr = metaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&runtimeInfo));
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// load the CLR (it won't be initialized just yet)
	hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&_runtimeHost));
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// hook up our unmanaged host to the runtime host
	assert(!_hostControl);
	_hostControl = new ClrHostControl();
	hr = _runtimeHost->SetHostControl(_hostControl);
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	ICLRControlPtr clrControl;
	hr = _runtimeHost->GetCLRControl(&clrControl);
	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// by default the CLR runtime will look for the app domain manager assembly in the same 
	// directory as the application, which in this case will be 
	// C:\Program Files\Unreal Engine\4.X\Engine\Binaries\Win64 (or Win32)
	hr = clrControl->SetAppDomainManagerType(
		L"Klawr.ClrHost.Managed", L"Klawr.ClrHost.Managed.DefaultAppDomainManager"
	);

	if (!verify(SUCCEEDED(hr)))
	{
		return false;
	}

	// initialize the CLR (not strictly necessary because the runtime can initialize itself)
	hr = _runtimeHost->Start();
	return SUCCEEDED(hr);
}

void __cdecl ClrHost::Shutdown()
{
	_hostControl->Shutdown();
	
	// NOTE: There's a crash here while debugging with the Mixed mode debugger, but everything works
	// fine when using the Auto mode debugger (which probably ends up using the Native debugger 
	// since this project is native). Everything also works fine if you detach the Mixed debugger 
	// before getting here.
	HRESULT hr = _runtimeHost->Stop();
	assert(SUCCEEDED(hr));

	if (_hostControl)
	{
		_hostControl->Release();
		_hostControl = nullptr;
	}
}
bool __cdecl ClrHost::CreateEngineAppDomain(int& outAppDomainID)
{
	outAppDomainID = _hostControl->GetDefaultAppDomainManager()->CreateEngineAppDomain(
		_engineAppDomainAppBase.c_str()
	);
	return _hostControl->GetEngineAppDomainManager(outAppDomainID) != nullptr;
}

bool __cdecl ClrHost::InitEngineAppDomain(int appDomainID, const NativeUtils& nativeUtils)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		// pass all the native wrapper functions to the managed side of the CLR host so that they 
		// can be hooked up to properties and methods of the generated C# wrapper classes (though 
		// that will happen a bit later)
		for (const auto& classWrapper : _classWrappers)
		{
			auto className = classWrapper.first.c_str();
			auto& wrapperInfo = classWrapper.second;
			auto safeArray = CreateSafeArrayOfWrapperFunctions(
				wrapperInfo.functionPointers, wrapperInfo.numFunctions
			);
			HRESULT hr = appDomainManager->SetNativeFunctionPointers(className, safeArray);
			assert(SUCCEEDED(hr));
		}

		// pass a few utility functions to the managed side
		appDomainManager->BindUtils(
			reinterpret_cast<Klawr::Managed::ObjectUtilsProxy*>(
				const_cast<ObjectUtilsProxy*>(&nativeUtils.Object)
			),
			reinterpret_cast<Klawr::Managed::LogUtilsProxy*>(
				const_cast<LogUtilsProxy*>(&nativeUtils.Log)
			),
			reinterpret_cast<Klawr::Managed::ArrayUtilsProxy*>(
				const_cast<ArrayUtilsProxy*>(&nativeUtils.Array)
			)
		);

		// now that everything the engine wrapper assembly needs is in place it can be loaded
		appDomainManager->LoadUnrealEngineWrapperAssembly();
		appDomainManager->LoadAssembly(_gameScriptsAssemblyName.c_str());
	}
	return appDomainManager != nullptr;
}

bool __cdecl ClrHost::DestroyEngineAppDomain(int appDomainID)
{
	return _hostControl->DestroyEngineAppDomain(appDomainID);
}

bool __cdecl ClrHost::CreateScriptObject(
	int appDomainID, const TCHAR* className, class UObject* owner, ScriptObjectInstanceInfo& info
)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (!appDomainManager)
	{
		return false;
	}

	Klawr::Managed::ScriptObjectInstanceInfo srcInfo;
	bool created = !!appDomainManager->CreateScriptObject(
		className, reinterpret_cast<INT_PTR>(owner), &srcInfo
	);
	if (created)
	{
		info.InstanceID = srcInfo.instanceID;
		info.BeginPlay = reinterpret_cast<ScriptObjectInstanceInfo::BeginPlayAction>(srcInfo.BeginPlay);
		info.Tick = reinterpret_cast<ScriptObjectInstanceInfo::TickAction>(srcInfo.Tick);
		info.Destroy = reinterpret_cast<ScriptObjectInstanceInfo::DestroyAction>(srcInfo.Destroy);
	}
	return created;
}

void __cdecl ClrHost::DestroyScriptObject(int appDomainID, __int64 instanceID)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->DestroyScriptObject(instanceID);
	}
}

bool __cdecl ClrHost::CreateScriptComponent(
	int appDomainID, const TCHAR* className, class UObject* nativeComponent, ScriptComponentProxy& proxy
)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (!appDomainManager)
	{
		return false;
	}

	return !!appDomainManager->CreateScriptComponent(
		className, reinterpret_cast<INT_PTR>(nativeComponent),
		reinterpret_cast<Klawr::Managed::ScriptComponentProxy*>(&proxy)
	);
}

void __cdecl ClrHost::DestroyScriptComponent(int appDomainID, __int64 instanceID)
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->DestroyScriptComponent(instanceID);
	}
}

void __cdecl ClrHost::GetScriptComponentTypes(int appDomainID, std::vector<tstring>& types) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* safeArray = appDomainManager->GetScriptComponentTypes();
		if (safeArray)
		{
			SafeArrayToVector<BSTR>(safeArray, types);
		}
	}
}

bool __cdecl ClrHost::GetScriptComponentPropertyIsAdvancedDisplay(int appDomainID, const TCHAR* typeName, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return (appDomainManager->GetScriptComponentPropertyIsAdvancedDisplay(typeName, propertyName) & 1) == 1;
	}
	return false;
}

bool __cdecl ClrHost::GetScriptComponentPropertyIsSaveGame(int appDomainID, const TCHAR* typeName, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return (appDomainManager->GetScriptComponentPropertyIsSaveGame(typeName, propertyName) & 1) == 1;
	}
	return false;
}

float __cdecl ClrHost::GetFloat(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return appDomainManager->GetFloat(instanceID, propertyName);
	}
	return 0.0f;
}

int __cdecl ClrHost::GetInt(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return appDomainManager->GetInt(instanceID, propertyName);
	}
	return 0;
}

bool __cdecl ClrHost::GetBool(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return appDomainManager->GetBool(instanceID, propertyName) == 1;
	}
	return false;
}

const TCHAR* __cdecl ClrHost::GetStr(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return appDomainManager->GetStr(instanceID, propertyName);
	}
	return TEXT("");
}

UObject* __cdecl ClrHost::GetObj(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return (UObject*)(appDomainManager->GetObj(instanceID, propertyName));
	}
	return NULL;
}

void __cdecl ClrHost::SetFloat(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, float value) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->SetFloat(instanceID, propertyName, value);
	}
}

void __cdecl ClrHost::SetInt(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, int32 value) const
{
		auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
		if (appDomainManager)
		{
			appDomainManager->SetInt(instanceID, propertyName, value);
		}
}

void __cdecl ClrHost::SetBool(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, bool value) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->SetBool(instanceID, propertyName, value);
	}
}

void __cdecl ClrHost::SetStr(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, const TCHAR* value) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->SetStr(instanceID, propertyName, value);
	}
}

void __cdecl ClrHost::SetObj(const int appDomainID, const __int64 instanceID, const TCHAR* propertyName, UObject* value) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		appDomainManager->SetObj(instanceID, propertyName, (long long)value);
	}
}

float __cdecl ClrHost::CallCSFunctionFloat(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<UObject*> objects) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* floatsArray = NULL;
		CreateSafeArray<float, VT_R4>(&(floats[0]), floats.size(), &floatsArray);
		SAFEARRAY* intsArray = NULL;
		CreateSafeArray<int, VT_I4>(&(ints[0]), ints.size(), &intsArray);
		SAFEARRAY* boolsArray = NULL;
		CreateSafeArrayBool(&bools, &boolsArray);
		SAFEARRAY* stringsArray = NULL;
		CreateSafeArrayString(&strings, &stringsArray);
		 

		return appDomainManager->CallCSFunctionFloat(instanceID, functionName, floatsArray, intsArray, boolsArray, stringsArray);
	}
	return 0.0f;
}

int __cdecl ClrHost::CallCSFunctionInt(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<UObject*> objects) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* floatsArray = NULL;
		CreateSafeArray<float, VT_R4>(&(floats[0]), floats.size(), &floatsArray);
		SAFEARRAY* intsArray = NULL;
		CreateSafeArray<int, VT_I4>(&(ints[0]), ints.size(), &intsArray);
		SAFEARRAY* boolsArray = NULL;
		CreateSafeArrayBool(&bools, &boolsArray);
		SAFEARRAY* stringsArray = NULL;
		CreateSafeArrayString(&strings, &stringsArray);


		return appDomainManager->CallCSFunctionInt(instanceID, functionName, floatsArray, intsArray, boolsArray, stringsArray);
	}
	return 0;
}

bool __cdecl ClrHost::CallCSFunctionBool(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<UObject*> objects) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* floatsArray = NULL;
		CreateSafeArray<float, VT_R4>(&(floats[0]), floats.size(), &floatsArray);
		SAFEARRAY* intsArray = NULL;
		CreateSafeArray<int, VT_I4>(&(ints[0]), ints.size(), &intsArray);
		SAFEARRAY* boolsArray = NULL;
		CreateSafeArrayBool(&bools, &boolsArray);
		SAFEARRAY* stringsArray = NULL;
		CreateSafeArrayString(&strings, &stringsArray);


		return appDomainManager->CallCSFunctionBool(instanceID, functionName, floatsArray, intsArray, boolsArray, stringsArray);
	}

	return false;
}

const TCHAR* __cdecl ClrHost::CallCSFunctionString(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<UObject*> objects) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* floatsArray = NULL;
		CreateSafeArray<float, VT_R4>(&(floats[0]), floats.size(), &floatsArray);
		SAFEARRAY* intsArray = NULL;
		CreateSafeArray<int, VT_I4>(&(ints[0]), ints.size(), &intsArray);
		SAFEARRAY* boolsArray = NULL;
		CreateSafeArrayBool(&bools, &boolsArray);
		SAFEARRAY* stringsArray = NULL;
		CreateSafeArrayString(&strings, &stringsArray);

		return appDomainManager->CallCSFunctionString(instanceID, functionName, floatsArray, intsArray, boolsArray, stringsArray);
	}

	return NULL;
}

UObject* __cdecl ClrHost::CallCSFunctionObject(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<UObject*> objects) const
{

	// Empty for now, dunno if i can implement it correctly  /!\

	return NULL;
}

void __cdecl ClrHost::CallCSFunctionVoid(int appDomainID, __int64 instanceID, const TCHAR* functionName, std::vector<float> floats, std::vector<int> ints, std::vector<bool> bools, std::vector<const TCHAR*> strings, std::vector<UObject*> objects) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		SAFEARRAY* floatsArray = NULL;
		CreateSafeArray<float, VT_R4>(&(floats[0]), floats.size(), &floatsArray);
		SAFEARRAY* intsArray = NULL;
		CreateSafeArray<int, VT_I4>(&(ints[0]), ints.size(), &intsArray);
		SAFEARRAY* boolsArray = NULL;
		CreateSafeArrayBool(&bools, &boolsArray);
		SAFEARRAY* stringsArray = NULL;
		CreateSafeArrayString(&strings, &stringsArray);

		appDomainManager->CallCSFunctionVoid(instanceID, functionName, floatsArray, intsArray, boolsArray, stringsArray);
	}
}

const TCHAR* __cdecl ClrHost::GetAssemblyInfo(int appDomainID) const
{
	auto appDomainManager = _hostControl->GetEngineAppDomainManager(appDomainID);
	if (appDomainManager)
	{
		return appDomainManager->GetAssemblyInfo();
	}
	return NULL;
}

void ClrHost::CreateSafeArrayBool(std::vector<bool>* bools, SAFEARRAY** boolsArray) const
{
	if (boolsArray == NULL)
	{
		// lpT and ppSafeArrayReceiver each cannot be NULL.
		return;
	}

	HRESULT hrRetTemp = S_OK;
	SAFEARRAYBOUND rgsabound[1];
	ULONG	ulIndex = 0;
	long lRet = 0;

	// Initialise receiver.
	*boolsArray = NULL;

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = bools->size();

	*boolsArray = (SAFEARRAY*)SafeArrayCreate
		(
			VT_BOOL,
			(unsigned int)1,
			(SAFEARRAYBOUND*)rgsabound
			);

	ulIndex = 0;
	for (std::vector<bool>::iterator it = bools->begin(); it != bools->end(); it++)
	{
		long lIndexVector[1];

		lIndexVector[0] = ulIndex;
		bool temp = *it;
		SafeArrayPutElement
			(
				(SAFEARRAY*)(*boolsArray),
				(long*)lIndexVector,
				(void*)(&(temp))
				);
		ulIndex++;
	}

	return;
}

void ClrHost::CreateSafeArrayString(std::vector<const TCHAR*>* strings, SAFEARRAY** stringsArray) const
{
	if (stringsArray == NULL)
	{
		// lpT and ppSafeArrayReceiver each cannot be NULL.
		return;
	}

	HRESULT hrRetTemp = S_OK;
	SAFEARRAYBOUND rgsabound[1];
	ULONG	ulIndex = 0;
	long lRet = 0;

	// Initialise receiver.
	*stringsArray = NULL;

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = strings->size();

	*stringsArray = (SAFEARRAY*)SafeArrayCreate
		(
			VT_BSTR,
			(unsigned int)1,
			(SAFEARRAYBOUND*)rgsabound
			);

	ulIndex = 0;
	for (std::vector<const TCHAR*>::iterator it = strings->begin(); it != strings->end(); it++)
	{
		long lIndexVector[1];

		lIndexVector[0] = ulIndex;
		BSTR temp = SysAllocString(*it);
		SafeArrayPutElement
			(
				(SAFEARRAY*)(*stringsArray),
				(long*)lIndexVector,
				(void*)(temp)
				);
		ulIndex++;
	}

	return;
}

} // namespace Klawr
